/*
 * (C) Copyright 2001-2003 Diomidis Spinellis.
 *
 * Web-based interface for viewing and processing C code
 *
 * $Id: cscout.cpp,v 1.85 2004/07/27 15:46:43 dds Exp $
 */

#include <map>
#include <string>
#include <deque>
#include <vector>
#include <stack>
#include <iterator>
#include <iostream>
#include <fstream>
#include <list>
#include <set>
#include <functional>
#include <algorithm>		// set_difference
#include <cctype>
#include <cassert>
#include <sstream>		// ostringstream
#include <cstdio>		// perror, rename
#include <cstdlib>		// atoi

#include "swill.h"
#include "regex.h"
#include "getopt.h"

#ifdef unix
#include <sys/types.h>		// mkdir
#include <sys/stat.h>		// mkdir
#include <unistd.h>		// unlink
#else
#include <io.h>			// mkdir
#endif

#include "cpp.h"
#include "ytab.h"
#include "attr.h"
#include "metrics.h"
#include "fileid.h"
#include "tokid.h"
#include "token.h"
#include "ptoken.h"
#include "fchar.h"
#include "error.h"
#include "pltoken.h"
#include "macro.h"
#include "pdtoken.h"
#include "eclass.h"
#include "debug.h"
#include "ctoken.h"
#include "type.h"
#include "stab.h"
#include "license.h"
#include "fdep.h"
#include "version.h"
#include "call.h"
#include "fcall.h"
#include "mcall.h"
#include "query.h"
#include "idquery.h"
#include "funquery.h"

#ifdef COMMERCIAL
#include "des.h"
#endif


// Global Web options
static bool remove_fp;			// Remove common file prefix
static bool show_true;			// Only show true identifier properties
static bool show_line_number;		// Annotate source with line numbers
static bool file_icase;			// File name case-insensitive match
static int tab_width = 8;		// Tab width for code output

// Global command-line options
static bool preprocess;			// Preprocess-only (-E)
static bool compile_only;		// Process-only (-c)
static bool report;			// Generate a warning report
static int portno = 8081;		// Port number (-p n)

static Fileid input_file_id;


// Identifiers to monitor (-m parameter)
static IdQuery monitor;

static IdProp ids;
static vector <Fileid> files;
static Attributes::size_type current_project;

void index_page(FILE *of, void *data);

// Display loop progress (non-reentant)
template <typename container>
static void
progress(typename container::const_iterator i, const container &c)
{
	static int count, opercent;

	if (i == c.begin())
		count = 0;
	int percent = ++count * 100 / c.size();
	if (percent != opercent) {
		cout << '\r' << percent << '%' << flush;
		opercent = percent;
	}
}

/*
 * Return as a C string the HTML equivalent of character c
 * Handles tab-stop expansion provided all output is processed through this
 * function
 */
static const char *
html(char c)
{
	static char str[2];
	static int column = 0;
	static vector<string> spaces(0);

	switch (c) {
	case '&': column++; return "&amp;";
	case '<': column++; return "&lt;";
	case '>': column++; return "&gt;";
	case ' ': column++; return "&nbsp;";
	case '\t':
		if ((int)(spaces.size()) != tab_width) {
			spaces.resize(tab_width);
			for (int i = 0; i < tab_width; i++) {
				string t;
				for (int j = 0; j < tab_width - i; j++)
					t += "&nbsp;";
				spaces[i] = t;
			}
		}
		return spaces[column % tab_width].c_str();
	case '\n':
		column = 0;
		return "<br>\n";
	default:
		column++;
		str[0] = c;
		return str;
	}
}

// HTML-encode the given string
static string
html(const string &s)
{
	string r;

	for (string::const_iterator i = s.begin(); i != s.end(); i++)
		r += html(*i);
	return r;
}

// Output s as HTML in of
static void
html_string(FILE *of, string s)
{
	for (string::const_iterator i = s.begin(); i != s.end(); i++)
		fputs(html(*i), of);
}

// Display an identifier hyperlink
static void
html(FILE *of, const IdPropElem &i)
{
	fprintf(of, "<a href=\"id.html?id=%p\">", i.first);
	html_string(of, (i.second).get_id());
	fputs("</a>", of);
}

static void
html(FILE *of, const Call &c)
{
	fprintf(of, "<a href=\"fun.html?f=%p\">", &c);
	html_string(of, c.get_name());
	fputs("</a>", of);
}


// Display a hyperlink based on a string and its starting tokid
static void
html_string(FILE *of, const string &s, Tokid t)
{
	int len = s.length();
	for (int pos = 0; pos < len;) {
		Eclass *ec = t.get_ec();
		Identifier id(ec, s.substr(pos, ec->get_len()));
		const IdPropElem ip(ec, id);
		html(of, ip);
		pos += ec->get_len();
		t += ec->get_len();
	}
}

// Add identifiers of the file fi into ids
// Return true if the file contains unused identifiers
static bool
file_analyze(Fileid fi)
{
	ifstream in;
	bool has_unused = false;
	const string &fname = fi.get_path();

	cout << "Post-processing " << fname << "\n";
	in.open(fname.c_str(), ios::binary);
	if (in.fail()) {
		perror(fname.c_str());
		exit(1);
	}
	// Go through the file character by character
	for (;;) {
		Tokid ti;
		int val;

		ti = Tokid(fi, in.tellg());
		if ((val = in.get()) == EOF)
			break;
		mapTokidEclass::iterator ei = ti.find_ec();
		if (ei != ti.end_ec()) {
			Eclass *ec = (*ei).second;
			// Remove identifiers we are not supposed to monitor
			if (monitor.is_valid()) {
				IdPropElem ec_id(ec, Identifier());
				if (!monitor.eval(ec_id)) {
					ec->remove_from_tokid_map();
					delete ec;
					continue;
				}
			}
			// Identifiers we can mark
			if (ec->is_identifier()) {
				// Update metrics
				id_msum.add_id(ec);
				// Add to the map
				string s;
				s = (char)val;
				int len = ec->get_len();
				for (int j = 1; j < len; j++)
					s += (char)in.get();
				fi.metrics().process_id(s);
				ids[ec] = Identifier(ec, s);
				if (ec->is_unused())
					has_unused = true;
				continue;
			} else {
				/*
				 * This equivalence class is not needed.
				 * (All potential identifier tokens,
				 * even reserved words get an EC. These are
				 * cleared here.)
				 */
				ec->remove_from_tokid_map();
				delete ec;
			}
		}
		fi.metrics().process_char((char)val);
		if ((char)val == '\n')
			fi.add_line_end(ti.get_streampos());
	}
	if (DP())
		cout << "nchar = " << fi.metrics().get_nchar() << '\n';
	in.close();
	return has_unused;
}

// Display the contents of a file in hypertext form
// Set show_unused to only mark unused identifiers
static void
file_hypertext(FILE *of, Fileid fi, bool eval_query)
{
	ifstream in;
	const string &fname = fi.get_path();
	IdQuery query(of, file_icase, current_project, eval_query);
	bool at_bol = true;
	int line_number = 1;

	if (DP())
		cout << "Write to " << fname << "\n";
	in.open(fname.c_str(), ios::binary);
	if (in.fail()) {
		perror(fname.c_str());
		exit(1);
	}
	fputs("<hr><code>", of);
	(void)html('\n');	// Reset HTML tab handling
	// Go through the file character by character
	for (;;) {
		Tokid ti;
		int val;

		ti = Tokid(fi, in.tellg());
		if ((val = in.get()) == EOF)
			break;
		if (at_bol) {
			fprintf(of,"<a name=\"%d\"></a>", line_number);
			if (show_line_number) {
				char buff[50];
				sprintf(buff, "%5d ", line_number);
				// Do not go via HTML string to keep tabs ok
				for (char *s = buff; *s; s++)
					if (*s == ' ')
						fputs("&nbsp;", of);
					else
						fputc(*s, of);
			}
			at_bol = false;
		}
		Eclass *ec;
		// Identifiers we can mark
		if ((ec = ti.check_ec()) && ec->is_identifier() && query.need_eval()) {
			string s;
			s = (char)val;
			int len = ec->get_len();
			for (int j = 1; j < len; j++)
				s += (char)in.get();
			Identifier i(ec, s);
			const IdPropElem ip(ec, i);
			if (query.eval(ip))
				html(of, ip);
			else
				html_string(of, s);
			continue;
		}
		fprintf(of, "%s", html((char)val));
		if ((char)val == '\n') {
			at_bol = true;
			line_number++;
		}
	}
	in.close();
	fputs("<hr></code>", of);
}

// Go through the file doing any replacements needed
// Return the numebr of replacements made
static int
file_replace(Fileid fid)
{
	string plain;
	ifstream in;
	ofstream out;

	in.open(fid.get_path().c_str(), ios::binary);
	if (in.fail()) {
		perror(fid.get_path().c_str());
		exit(1);
	}
	string ofname = fid.get_path() + ".repl";
	out.open(ofname.c_str(), ios::binary);
	if (out.fail()) {
		perror(ofname.c_str());
		exit(1);
	}
	cout << "Processing file " << fid.get_path() << "\n";
	int replacements = 0;
	// Go through the file character by character
	for (;;) {
		Tokid ti;
		int val;

		ti = Tokid(fid, in.tellg());
		if ((val = in.get()) == EOF)
			break;
		Eclass *ec;
		IdProp::const_iterator idi;
		// Identifiers that should be replaced
		if ((ec = ti.check_ec()) &&
		    ec->is_identifier() &&
		    (idi = ids.find(ec)) != ids.end() &&
		    (*idi).second.get_replaced()) {
			int len = ec->get_len();
			for (int j = 1; j < len; j++)
				(void)in.get();
			out << (*idi).second.get_newid();
			replacements++;
		} else {
			out << (char)val;
		}
	}
	// Needed for Windows
	in.close();
	out.close();
	// Should actually be an assertion
	if (replacements) {
		string cmd("cscout_checkout " + fid.get_path());
		system(cmd.c_str());
		unlink(fid.get_path().c_str());
		rename(ofname.c_str(), fid.get_path().c_str());
		string cmd2("cscout_checkin " + fid.get_path());
		system(cmd2.c_str());
	}
	return replacements;
}

// Create a new HTML file with a given filename and title
static void
html_head(FILE *of, const string fname, const string title)
{
	swill_title(title.c_str());
	if (DP())
		cerr << "Write to " << fname << "\n";
	fprintf(of,
		"<!doctype html public \"-//IETF//DTD HTML//EN\">\n"
		"<html>\n"
		"<head>\n"
		"<meta name=\"GENERATOR\" content=\"CScout %s - %s\">\n"
		"<title>%s</title>\n"
		"</head>\n"
		"<body>\n"
		"<h1>%s</h1>\n",
		Version::get_revision().c_str(),
		Version::get_date().c_str(),
		title.c_str(), title.c_str());
}

// And an HTML file end
static void
html_tail(FILE *of)
{
	if (current_project)
		fprintf(of, "<p> <b>Project %s is currently selected</b>\n", Project::get_projname(current_project).c_str());
	fprintf(of,
		"<p>"
		"<a href=\"index.html\">Main page</a>\n"
		" - Web: "
		"<a href=\"http://www.spinellis.gr/cscout\">Home</a>\n"
		"<a href=\"http://www.spinellis.gr/cscout/doc/index.html\">Manual</a>\n"
		"<br><hr><font size=-1>CScout %s - %s</font>\n"
		"</body>"
		"</html>\n", Version::get_revision().c_str(),
		Version::get_date().c_str());
}

#ifndef COMMERCIAL
/*
 * Return TRUE if the access if from the local host
 * Used to safeguard dangerous operations such as renaming and exiting
 */
static bool
local_access(FILE *fo)
{
	char *peer = swill_getpeerip();

	if (peer && strcmp(peer, "127.0.0.1") == 0)
		return true;
	else {
		html_head(fo, "Remote access", "Remote access not allowed");
		fprintf(fo, "This function can not be executed from a remote host.");
		fprintf(fo, "Make sure you are accessing cscout as localhost or 127.0.0.1.");
		html_tail(fo);
		return false;
	}
}
#endif

static bool html_file_starting;
static string odir;

static void
html_file_begin(FILE *of)
{
	html_file_starting = true;
	fprintf(of, "<ul>\n");
}

static void
html_file_end(FILE *of)
{
	if (remove_fp && !html_file_starting)
		fprintf(of, "</ul>\n");
	fprintf(of, "</ul>\n");
}

// Display a filename of an html file
static void
html_file(FILE *of, Fileid fi)
{
	if (!remove_fp) {
		fprintf(of, "\n<li><a href=\"file.html?id=%u\">%s</a>",
			fi.get_id(),
			fi.get_path().c_str());
		return;
	}

	// Split path into dir and fname
	string s(fi.get_path());
	string::size_type k = s.find_last_of("/\\");
	if (k == s.npos)
		k = 0;
	else
		k++;
	string dir(s, 0, k);
	string fname(s, k);

	if (html_file_starting) {
		html_file_starting = false;
		fprintf(of, "<li>%s\n<ul>\n", dir.c_str());
		odir = dir;
	} else if (odir != dir) {
		fprintf(of, "</ul><li>%s\n<ul>\n", dir.c_str());
		odir = dir;
	}
	fprintf(of, "\n<li><a href=\"file.html?id=%u\">%s</a>",
		fi.get_id(),
		fname.c_str());
}

static void
html_file(FILE *of, string fname)
{
	Fileid fi = Fileid(fname);

	html_file(of, fi);
}

// File query page
static void
fquery_page(FILE *of,  void *p)
{
	html_head(of, "fquery", "File Query");
	fputs("<FORM ACTION=\"xfquery.html\" METHOD=\"GET\">\n"
	"<input type=\"checkbox\" name=\"writable\" value=\"1\">Writable<br>\n"
	"<input type=\"checkbox\" name=\"ro\" value=\"1\">Read-only<br>\n"
	"<table>", of);
	for (int i = 0; i < metric_max; i++) {
		fprintf(of, "<tr><td>%s</td><td><select name=\"c%d\" value=\"1\">\n",
			Metrics::name(i).c_str(), i);
		Query::equality_selection(of);
		fprintf(of, "</td><td><INPUT TYPE=\"text\" NAME=\"n%d\" SIZE=5 MAXLENGTH=10></td></tr>\n", i);
	}
	fputs(
	"</table><p>\n"
	"<input type=\"radio\" name=\"match\" value=\"Y\" CHECKED>Match any of the above\n"
	"&nbsp; &nbsp; &nbsp; &nbsp;\n"
	"<input type=\"radio\" name=\"match\" value=\"L\">Match all of the above\n"
	"<br><hr>\n"
	"File names should match RE\n"
	"<INPUT TYPE=\"text\" NAME=\"fre\" SIZE=20 MAXLENGTH=256>\n"
	"<hr>\n"
	"<p>Query title <INPUT TYPE=\"text\" NAME=\"n\" SIZE=60 MAXLENGTH=256>\n"
	"&nbsp;&nbsp;<INPUT TYPE=\"submit\" NAME=\"qf\" VALUE=\"Show files\">\n"
	"</FORM>\n"
	, of);
	html_tail(of);
}

struct ignore : public binary_function <int, int, bool> {
	inline bool operator()(int a, int b) const { return true; }
};


// Process a file query
static void
xfquery_page(FILE *of,  void *p)
{
	IFSet sorted_files;
	char match_type;
	vector <int> op(metric_max);
	vector <int> n(metric_max);
	bool writable = !!swill_getvar("writable");
	bool ro = !!swill_getvar("ro");
	char *qname = swill_getvar("n");

	html_head(of, "xfquery", (qname && *qname) ? qname : "File Query Results");

	char *m;
	if (!(m = swill_getvar("match"))) {
		fprintf(of, "Missing value: match");
		return;
	}
	match_type = *m;

	// Compile regular expression specs
	regex_t fre;
	bool match_fre;
	char *s;
	int r;
	match_fre = false;
	if ((s = swill_getvar("fre")) && *s) {
		match_fre = true;
		if ((r = regcomp(&fre, s, REG_EXTENDED | REG_NOSUB | (file_icase ? REG_ICASE : 0)))) {
			char buff[1024];
			regerror(r, &fre, buff, sizeof(buff));
			fprintf(of, "<h2>Filename regular expression error</h2>%s", buff);
			html_tail(of);
			return;
		}
	}

	// Store metric specifications in a vector
	for (int i = 0; i < metric_max; i++) {
		ostringstream argspec;

		argspec << "|i(c" << i << ")";
		argspec << "i(n" << i << ")";
		op[i] = n[i] = 0;
		(void)swill_getargs(argspec.str().c_str(), &(op[i]), &(n[i]));
	}

	for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++) {
		if (current_project && !(*i).get_attribute(current_project))
			continue;
		if (match_fre && regexec(&fre, (*i).get_path().c_str(), 0, NULL, 0) == REG_NOMATCH)
			continue;

		bool add;
		switch (match_type) {
		case 'Y':	// anY match
			add = false;
			for (int j = 0; j < metric_max; j++)
				if (op[j] && Query::apply(op[j], (*i).metrics().get_metric(j), n[j])) {
					add = true;
					break;
				}
			add = (add || (ro && (*i).get_readonly()));
			add = (add || (writable && !(*i).get_readonly()));
			break;
		case 'L':	// alL match
			add = true;
			for (int j = 0; j < metric_max; j++)
				if (op[j] && !Query::apply(op[j], (*i).metrics().get_metric(j), n[j])) {
					add = false;
					break;
				}
			add = (add && (!ro || (*i).get_readonly()));
			add = (add && (!writable || !(*i).get_readonly()));
			break;
		}
		if (add)
			sorted_files.insert(*i);
	}
	html_file_begin(of);
	for (IFSet::iterator i = sorted_files.begin(); i != sorted_files.end(); i++) {
		Fileid f = *i;
		if (current_project && !f.get_attribute(current_project))
			continue;
		html_file(of, *i);
	}
	html_file_end(of);
	fprintf(of, "\n</ul>\n");
	fputs("<p>You can bookmark this page to save the respective query<p>", of);
	html_tail(of);
	if (match_fre)
		regfree(&fre);
}


/*
 * Display the sorted identifiers, taking into account the reverse sort property
 * for properly aligning the output.
 */
template <typename container>
static void
display_sorted(FILE *of, const container &sorted_ids)
{
	if (Query::sort_rev)
		fputs("<table><tr><td width=\"50%\" align=\"right\">\n", of);
	else
		fputs("<p>\n", of);

	typename container::const_iterator i;
	for (i = sorted_ids.begin(); i != sorted_ids.end(); i++) {
		html(of, **i);
		fputs("<br>\n", of);
	}

	if (Query::sort_rev)
		fputs("</td> <td width=\"50%\"> </td></tr></table>\n", of);
	else
		fputs("</p>\n", of);
}

// Identifier query page
static void
iquery_page(FILE *of,  void *p)
{
	html_head(of, "iquery", "Identifier Query");
	fputs("<FORM ACTION=\"xiquery.html\" METHOD=\"GET\">\n"
	"<input type=\"checkbox\" name=\"writable\" value=\"1\">Writable<br>\n", of);
	for (int i = attr_begin; i < attr_end; i++)
		fprintf(of, "<input type=\"checkbox\" name=\"a%d\" value=\"1\">%s<br>\n", i,
			Attributes::name(i).c_str());
	fputs(
	"<input type=\"checkbox\" name=\"xfile\" value=\"1\">Crosses file boundary<br>\n"
	"<input type=\"checkbox\" name=\"unused\" value=\"1\">Unused<br>\n"
	"<p>\n"
	"<input type=\"radio\" name=\"match\" value=\"Y\" CHECKED>Match any marked\n"
	"&nbsp; &nbsp; &nbsp; &nbsp;\n"
	"<input type=\"radio\" name=\"match\" value=\"L\">Match all marked\n"
	"&nbsp; &nbsp; &nbsp; &nbsp;\n"
	"<input type=\"radio\" name=\"match\" value=\"E\">Exclude marked\n"
	"&nbsp; &nbsp; &nbsp; &nbsp;\n"
	"<input type=\"radio\" name=\"match\" value=\"T\" >Exact match\n"
	"<br><hr>\n"
	"<table>\n"
	"<tr><td>\n"
	"Identifier names should "
	"(<input type=\"checkbox\" name=\"xire\" value=\"1\"> not) \n"
	" match RE\n"
	"</td><td>\n"
	"<INPUT TYPE=\"text\" NAME=\"ire\" SIZE=20 MAXLENGTH=256>\n"
	"</td></tr>\n"
	"<tr><td>\n"
	"Select identifiers from filenames matching RE\n"
	"</td><td>\n"
	"<INPUT TYPE=\"text\" NAME=\"fre\" SIZE=20 MAXLENGTH=256>\n"
	"</td></tr>\n"
	"</table>\n"
	"<hr>\n"
	"<p>Query title <INPUT TYPE=\"text\" NAME=\"n\" SIZE=60 MAXLENGTH=256>\n"
	"&nbsp;&nbsp;<INPUT TYPE=\"submit\" NAME=\"qi\" VALUE=\"Show identifiers\">\n"
	"<INPUT TYPE=\"submit\" NAME=\"qf\" VALUE=\"Show files\">\n"
	"</FORM>\n"
	, of);
	html_tail(of);
}

// Function query page
static void
funquery_page(FILE *of,  void *p)
{
	html_head(of, "funquery", "Function Query");
	fputs("<FORM ACTION=\"xfunquery.html\" METHOD=\"GET\">\n"
	"<input type=\"checkbox\" name=\"cfun\" value=\"1\">C function<br>\n"
	"<input type=\"checkbox\" name=\"macro\" value=\"1\">Function-like macro<br>\n"
	"<input type=\"checkbox\" name=\"writable\" value=\"1\">Writable declaration<br>\n"
	"<input type=\"checkbox\" name=\"ro\" value=\"1\">Read-only declaration<br>\n"
	"<input type=\"checkbox\" name=\"pscope\" value=\"1\">Project scope<br>\n"
	"<input type=\"checkbox\" name=\"fscope\" value=\"1\">File scope<br>\n"
	"<input type=\"checkbox\" name=\"defined\" value=\"1\">Defined<br>\n"
	"<p>\n"
	"<input type=\"radio\" name=\"match\" value=\"Y\" CHECKED>Match any marked\n"
	"&nbsp; &nbsp; &nbsp; &nbsp;\n"
	"<input type=\"radio\" name=\"match\" value=\"L\">Match all marked\n"
	"&nbsp; &nbsp; &nbsp; &nbsp;\n"
	"<input type=\"radio\" name=\"match\" value=\"E\">Exclude marked\n"
	"&nbsp; &nbsp; &nbsp; &nbsp;\n"
	"<input type=\"radio\" name=\"match\" value=\"T\" >Exact match\n"
	"<br><hr>\n"
	"<table>\n"

	"<tr><td>\n"
	"Number of direct callers\n"
	"<select name=\"ncallerop\" value=\"1\">\n",
	of);
	Query::equality_selection(of);
	fputs(
	"</td><td>\n"
	"<INPUT TYPE=\"text\" NAME=\"ncallers\" SIZE=5 MAXLENGTH=10>\n"
	"</td><td>\n"

	"<tr><td>\n"
	"Function names should "
	"(<input type=\"checkbox\" name=\"xfnre\" value=\"1\"> not) \n"
	" match RE\n"
	"</td><td>\n"
	"<INPUT TYPE=\"text\" NAME=\"fnre\" SIZE=20 MAXLENGTH=256>\n"
	"</td></tr>\n"

	"<tr><td>\n"
	"Names of calling functions should "
	"(<input type=\"checkbox\" name=\"xfure\" value=\"1\"> not) \n"
	" match RE\n"
	"</td><td>\n"
	"<INPUT TYPE=\"text\" NAME=\"fure\" SIZE=20 MAXLENGTH=256>\n"
	"</td></tr>\n"

	"<tr><td>\n"
	"Names of called functions should "
	"(<input type=\"checkbox\" name=\"xfdre\" value=\"1\"> not) \n"
	" match RE\n"
	"</td><td>\n"
	"<INPUT TYPE=\"text\" NAME=\"fdre\" SIZE=20 MAXLENGTH=256>\n"
	"</td></tr>\n"

	"<tr><td>\n"
	"Select functions from filenames matching RE\n"
	"</td><td>\n"
	"<INPUT TYPE=\"text\" NAME=\"fre\" SIZE=20 MAXLENGTH=256>\n"
	"</td></tr>\n"
	"</table>\n"
	"<hr>\n"
	"<p>Query title <INPUT TYPE=\"text\" NAME=\"n\" SIZE=60 MAXLENGTH=256>\n"
	"&nbsp;&nbsp;<INPUT TYPE=\"submit\" NAME=\"qi\" VALUE=\"Show functions\">\n"
	"<INPUT TYPE=\"submit\" NAME=\"qf\" VALUE=\"Show files\">\n"
	"</FORM>\n"
	, of);
	html_tail(of);
}
void
display_files(FILE *of, const Query &query, const IFSet &sorted_files)
{
	const string query_url(query.url());

	fputs("<h2>Matching Files</h2>\n", of);
	html_file_begin(of);
	for (IFSet::iterator i = sorted_files.begin(); i != sorted_files.end(); i++) {
		Fileid f = *i;
		if (current_project && !f.get_attribute(current_project))
			continue;
		html_file(of, *i);
		fprintf(of, " - <a href=\"qsrc.html?id=%u&%s\">marked source</a>",
			f.get_id(),
			query_url.c_str());
	}
	html_file_end(of);
}

// Process an identifier query
static void
xiquery_page(FILE *of,  void *p)
{
	Sids sorted_ids;
	IFSet sorted_files;
	bool q_id = !!swill_getvar("qi");	// Show matching identifiers
	bool q_file = !!swill_getvar("qf");	// Show matching files
	char *qname = swill_getvar("n");
	IdQuery query(of, file_icase, current_project);

#ifndef COMMERCIAL
		if (!local_access(of))
			return;
#endif

	if (!query.is_valid()) {
		html_tail(of);
		return;
	}

	html_head(of, "xiquery", (qname && *qname) ? qname : "Identifier Query Results");
	cout << "Evaluating identifier query\n";
	for (IdProp::iterator i = ids.begin(); i != ids.end(); i++) {
		progress(i, ids);
		if (!query.eval(*i))
			continue;
		if (q_id)
			sorted_ids.insert(&*i);
		if (q_file) {
			IFSet f = (*i).first->sorted_files();
			sorted_files.insert(f.begin(), f.end());
		}
	}
	cout << '\n';
	if (q_id) {
		fputs("<h2>Matching Identifiers</h2>\n", of);
		display_sorted(of, sorted_ids);
	}
	if (q_file)
		display_files(of, query, sorted_files);
	fputs("<p>You can bookmark this page to save the respective query<p>", of);
	html_tail(of);
}

// Process a funcion query
static void
xfunquery_page(FILE *of,  void *p)
{
	Sfuns sorted_funs;
	IFSet sorted_files;
	bool q_id = !!swill_getvar("qi");	// Show matching identifiers
	bool q_file = !!swill_getvar("qf");	// Show matching files
	char *qname = swill_getvar("n");
	FunQuery query(of, file_icase, current_project);

#ifndef COMMERCIAL
		if (!local_access(of))
			return;
#endif

	if (!query.is_valid())
		return;

	html_head(of, "xfunquery", (qname && *qname) ? qname : "Function Query Results");
	cout << "Evaluating function query\n";
	for (Call::const_fmap_iterator_type i = Call::fbegin(); i != Call::fend(); i++) {
		progress(i, Call::functions());
		if (!query.eval(i->second))
			continue;
		if (q_id)
			sorted_funs.insert(i->second);
		if (q_file)
			sorted_files.insert(i->second->get_fileid());
	}
	cout << '\n';
	if (q_id) {
		fputs("<h2>Matching Functions</h2>\n", of);
		display_sorted(of, sorted_funs);
	}
	if (q_file)
		// XXX Move to a function
		display_files(of, query, sorted_files);
	fputs("<p>You can bookmark this page to save the respective query<p>", of);
	html_tail(of);
}

// Display an identifier property
static void
show_id_prop(FILE *fo, const string &name, bool val)
{
	if (!show_true || val)
		fprintf(fo, ("<li>" + name + ": %s\n").c_str(), val ? "Yes" : "No");
}

// Details for each identifier
void
identifier_page(FILE *fo, void *p)
{
	Eclass *e;
	if (!swill_getargs("p(id)", &e)) {
		fprintf(fo, "Missing value");
		return;
	}
	char *subst;
	Identifier &id = ids[e];
	if ((subst = swill_getvar("sname"))) {
#ifndef COMMERCIAL
		if (!local_access(fo))
			return;
#endif
		// Passing subst directly core-dumps under
		// gcc version 2.95.4 20020320 [FreeBSD 4.7]
		string ssubst(subst);
		id.set_newid(ssubst);
	}
	html_head(fo, "id", string("Identifier: ") + html(id.get_id()));
	fprintf(fo, "<FORM ACTION=\"id.html\" METHOD=\"GET\">\n<ul>\n");
	for (int i = attr_begin; i < attr_end; i++)
		show_id_prop(fo, Attributes::name(i), e->get_attribute(i));
	show_id_prop(fo, "Crosses file boundary", id.get_xfile());
	show_id_prop(fo, "Unused", e->is_unused());
	fprintf(fo, "<li> Matches %d occurence(s)\n", e->get_size());
	fprintf(fo, "<li> Appears in project(s): \n<ul>\n");
	if (DP()) {
		cout << "First project " << attr_end << "\n";
		cout << "Last project " <<  Attributes::get_num_attributes() - 1 << "\n";
	}
	for (Attributes::size_type j = attr_end; j < Attributes::get_num_attributes(); j++)
		if (e->get_attribute(j))
			fprintf(fo, "<li>%s\n", Project::get_projname(j).c_str());
	fprintf(fo, "</ul>\n");
	if (e->get_attribute(is_function) || e->get_attribute(is_macro)) {
		bool found = false;
		for (Call::const_fmap_iterator_type i = Call::fbegin(); i != Call::fend(); i++) {
			if (i->second->contains(e)) {
				if (!found) {
					fprintf(fo, "<li> The identifier occurs (wholy or in part) in function name(s): \n<ol>\n");
					found = true;
				}
				fprintf(fo, "\n<li>");
				html_string(fo, i->second->get_name(), i->first);
				fprintf(fo, " - <a href=\"fun.html?f=%p\">function page</a>", i->second);
			}
		}
		if (found)
			fprintf(fo, "</ol><br />\n");
	}
	if (id.get_replaced())
		fprintf(fo, "<li> Substituted with: [%s]\n", id.get_newid().c_str());
	if (!e->get_attribute(is_readonly)) {
		fprintf(fo, "<li> Substitute with: \n"
			"<INPUT TYPE=\"text\" NAME=\"sname\" SIZE=10 MAXLENGTH=256> "
			"<INPUT TYPE=\"submit\" NAME=\"repl\" VALUE=\"Substitute\">\n");
		fprintf(fo, "<INPUT TYPE=\"hidden\" NAME=\"id\" VALUE=\"%p\">\n", e);
	}
	fprintf(fo, "</ul>\n");

	IFSet ifiles = e->sorted_files();
	fprintf(fo, "<h2>Dependent Files (Writable)</h2>\n");
	html_file_begin(fo);
	for (IFSet::const_iterator j = ifiles.begin(); j != ifiles.end(); j++)
		if ((*j).get_readonly() == false) {
			html_file(fo, (*j).get_path());
			fprintf(fo, " - <a href=\"qsrc.html?id=%u&ec=%p&n=Identifier+%s\">marked source</a>",
				(*j).get_id(),
				e, id.get_id().c_str());
		}
	html_file_end(fo);
	fprintf(fo, "<h2>Dependent Files (All)</h2>\n");
	html_file_begin(fo);
	for (IFSet::const_iterator j = ifiles.begin(); j != ifiles.end(); j++) {
		html_file(fo, (*j).get_path());
		fprintf(fo, " - <a href=\"qsrc.html?id=%u&ec=%p&n=Identifier+%s\">marked source</a>",
			(*j).get_id(),
			e, id.get_id().c_str());
	}
	html_file_end(fo);
	fprintf(fo, "</FORM>\n");
	html_tail(fo);
}

// Details for each function
void
function_page(FILE *fo, void *p)
{
	Call *f;
	if (!swill_getargs("p(f)", &f)) {
		fprintf(fo, "Missing value");
		return;
	}
	html_head(fo, "fun", string("Function: ") + html(f->get_name()) + " (" + f->entity_type_name() + ')');
	fprintf(fo, "<ul>\n");
	fprintf(fo, "<li> Associated identifier: ");
	Tokid t = f->get_tokid();
	html_string(fo, f->get_name(), t);
	if (f->is_declared()) {
		fprintf(fo, "\n<li> Declared in file <a href=\"file.html?id=%u\">%s</a>",
			t.get_fileid().get_id(),
			t.get_fileid().get_path().c_str());
		ostringstream fname;
		fname << t.get_fileid().get_id();
		int lnum = t.get_fileid().line_number(t.get_streampos());
		fprintf(fo, " <a href=\"src.html?id=%s#%d\">line %d</a><br />(and possibly in other places)\n",
			fname.str().c_str(), lnum, lnum);
	}
	if (f->is_defined()) {
		t = f->get_definition();
		fprintf(fo, "<li> Defined in file <a href=\"file.html?id=%u\">%s</a>",
			t.get_fileid().get_id(),
			t.get_fileid().get_path().c_str());
		ostringstream fname;
		fname << t.get_fileid().get_id();
		int lnum = t.get_fileid().line_number(t.get_streampos());
		fprintf(fo, " <a href=\"src.html?id=%s#%d\">line %d</a>\n",
			fname.str().c_str(), lnum, lnum);
	} else
		fprintf(fo, "<li> No definition found\n");
	// Functions that are Down from us in the call graph
	fprintf(fo, "<li> Calls directly %d functions", f->get_num_call());
	fprintf(fo, "<li> <a href=\"funlist.html?f=%p&n=d\">List of directly called functions</a>\n", f);
	fprintf(fo, "<li> <a href=\"funlist.html?f=%p&n=D\">List of all called functions</a>\n", f);
	// Functions that are Up from us in the call graph
	fprintf(fo, "<li> Called directly by %d functions", f->get_num_caller());
	fprintf(fo, "<li> <a href=\"funlist.html?f=%p&n=u\">List of direct callers</a>\n", f);
	fprintf(fo, "<li> <a href=\"funlist.html?f=%p&n=U\">List of all callers</a>\n", f);
	fprintf(fo, "</ul>\n");
	html_tail(fo);
}

/*
 * List all functions associated with a call/caller relationship with f
 * The methods to obtain the relationship containers are passed through
 * the fbegin and fend method pointers.
 * If recurse is true the also list will contain all correspondingly
 * associated children functions.
 */
void
list_functions(FILE *fo, Call *f,
	Call::const_fiterator_type (Call::*fbegin)() const,
	Call::const_fiterator_type (Call::*fend)() const,
	bool recurse)
{
	Call::const_fiterator_type i;

	f->set_visited();
	for (i = (f->*fbegin)(); i != (f->*fend)(); i++) {
		if (!(*i)->is_visited() || *i == f) {
			fprintf(fo, "<li> ");
			html(fo, **i);
		}
		if (recurse && !(*i)->is_visited())
			list_functions(fo, *i, fbegin, fend, recurse);
	}
}

// List of functions associated with a given one
void
funlist_page(FILE *fo, void *p)
{
	Call *f;
	if (!swill_getargs("p(f)", &f)) {
		fprintf(fo, "Missing value");
		return;
	}
	char *ltype = swill_getvar("n");
	html_head(fo, "funlist", "Function List");
	fprintf(fo, "<h2>Function ");
	html(fo, *f);
	fprintf(fo, "</h2>");
	char *calltype;
	bool recurse;
	switch (*ltype) {
	case 'u': case 'd':
		calltype = "directly";
		recurse = false;
		break;
	case 'D': case 'U':
		calltype = "all";
		recurse = true;
		break;
	default:
		fprintf(fo, "Illegal value");
		return;
	}
	// Pointers to the ...begin and ...end methods
	Call::const_fiterator_type (Call::*fbegin)() const;
	Call::const_fiterator_type (Call::*fend)() const;
	switch (*ltype) {
	case 'u':
	case 'U':
		fbegin = &Call::caller_begin;
		fend = &Call::caller_end;
		fprintf(fo, "List of %s calling functions\n", calltype);
		break;
	case 'd':
	case 'D':
		fbegin = &Call::call_begin;
		fend = &Call::call_end;
		fprintf(fo, "List of %s called functions\n", calltype);
		break;
	}
	fprintf(fo, "<ul>\n");
	Call::clear_visit_flags();
	list_functions(fo, f, fbegin, fend, recurse);
	fprintf(fo, "</ul>\n");
	html_tail(fo);
}


// Front-end global options page
void
options_page(FILE *fo, void *p)
{
	html_head(fo, "options", "Global Options");
	fprintf(fo, "<FORM ACTION=\"soptions.html\" METHOD=\"GET\">\n");
	fprintf(fo, "<input type=\"checkbox\" name=\"remove_fp\" value=\"1\" %s>Remove common path prefix in file lists<br>\n", (remove_fp ? "checked" : ""));
	fprintf(fo, "<input type=\"checkbox\" name=\"sort_rev\" value=\"1\" %s>Sort identifiers starting from their last character<br>\n", (Query::sort_rev ? "checked" : ""));
	fprintf(fo, "<input type=\"checkbox\" name=\"show_true\" value=\"1\" %s>Show only true identifier classes (brief view)<br>\n", (show_true ? "checked" : ""));
	fprintf(fo, "<input type=\"checkbox\" name=\"show_line_number\" value=\"1\" %s>Show line numbers in source listings<br>\n", (show_line_number ? "checked" : ""));
	fprintf(fo, "<input type=\"checkbox\" name=\"file_icase\" value=\"1\" %s>Case-insensitive file name regular expression match<br>\n", (file_icase ? "checked" : ""));
	fprintf(fo, "<p>Code listing tab width <input type=\"text\" name=\"tab_width\" size=3 maxlength=3 value=\"%d\"><br>\n", tab_width);
/*
Do not show No in identifier properties (option)

*/
	fprintf(fo, "<p><p><INPUT TYPE=\"submit\" NAME=\"set\" VALUE=\"OK\">\n");
	fprintf(fo, "<INPUT TYPE=\"submit\" NAME=\"set\" VALUE=\"Cancel\">\n");
	fprintf(fo, "<INPUT TYPE=\"submit\" NAME=\"set\" VALUE=\"Apply\">\n");
	fprintf(fo, "</FORM>\n");
	html_tail(fo);
}

// Front-end global options page
void
set_options_page(FILE *fo, void *p)
{
	if (string(swill_getvar("set")) == "Cancel") {
		index_page(fo, p);
		return;
	}
	remove_fp = !!swill_getvar("remove_fp");
	Query::sort_rev = !!swill_getvar("sort_rev");
	show_true = !!swill_getvar("show_true");
	show_line_number = !!swill_getvar("show_line_number");
	file_icase = !!swill_getvar("file_icase");
	if (!swill_getargs("I(tab_width)", &tab_width) || tab_width <= 0)
		tab_width = 8;
	if (string(swill_getvar("set")) == "Apply")
		options_page(fo, p);
	else
		index_page(fo, p);
}

void
file_metrics_page(FILE *fo, void *p)
{
	html_head(fo, "fmetrics", "File Metrics");
	ostringstream mstring;
	mstring << file_msum;
	fputs(mstring.str().c_str(), fo);
	html_tail(fo);
}

void
id_metrics_page(FILE *fo, void *p)
{
	html_head(fo, "idmetrics", "Identifier Metrics");
	ostringstream mstring;
	mstring << id_msum;
	fputs(mstring.str().c_str(), fo);
	html_tail(fo);
}

// Call graph
void
cgraph_page(FILE *fo, void *p)
{
	html_head(fo, "cgraph", "Call Graph");
	Call::const_fmap_iterator_type fun;
	Call::const_fiterator_type call;
	for (fun = Call::fbegin(); fun != Call::fend(); fun++) {
		fprintf(fo, "%s:%s <br />\n",
		    fun->second->get_site().get_fileid().get_path().c_str(),
		    fun->second->get_name().c_str());
	for (call = fun->second->call_begin(); call != fun->second->call_end(); call++)
			fprintf(fo, "&nbsp;&nbsp;&nbsp;&nbsp; calls %s:%s <br />\n",
			    (*call)->get_site().get_fileid().get_path().c_str(),
			    (*call)->get_name().c_str());
		for (call = fun->second->caller_begin(); call != fun->second->caller_end(); call++)
			fprintf(fo, "&nbsp;&nbsp;&nbsp;&nbsp; called-by %s:%s <br />\n",
			    (*call)->get_site().get_fileid().get_path().c_str(),
			    (*call)->get_name().c_str());
	}
	html_tail(fo);
}

// Display all projects, allowing user to select
void
select_project_page(FILE *fo, void *p)
{
	html_head(fo, "sproject", "Select Active Project");
	fprintf(fo, "<ul>\n");
	fprintf(fo, "<li> <a href=\"setproj.html?projid=0\">All projects</a>\n");
	for (Attributes::size_type j = attr_end; j < Attributes::get_num_attributes(); j++)
		fprintf(fo, "<li> <a href=\"setproj.html?projid=%u\">%s</a>\n", (unsigned)j, Project::get_projname(j).c_str());
	fprintf(fo, "\n</ul>\n");
	html_tail(fo);
}

// Select a single project (or none) to restrict file/identifier results
void
set_project_page(FILE *fo, void *p)
{
#ifndef COMMERCIAL
	if (!local_access(fo))
		return;
#endif

	if (!swill_getargs("i(projid)", &current_project)) {
		fprintf(fo, "Missing value");
		return;
	}
	index_page(fo, p);
}


// Index
void
index_page(FILE *of, void *data)
{
	html_head(of, "index", "CScout Main Page");
	fprintf(of,
		"<h2>Files</h2>\n"
		"<ul>\n"
		"<li> <a href=\"fmetrics.html\">File Metrics</a>\n"
		"<li> <a href=\"xfquery.html?ro=1&writable=1&match=Y&n=All+Files&qf=1\">All files</a>\n"
		"<li> <a href=\"xfquery.html?ro=1&match=Y&n=Read-only+Files&qf=1\">Read-only files</a>\n"
		"<li> <a href=\"xfquery.html?writable=1&match=Y&n=Writable+Files&qf=1\">Writable files</a>\n");
	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&unused=1&match=L&qf=1&n=Files+Containing+Unused+Project-scoped+Writable+Identifiers\">Files containing unused project-scoped writable identifiers</a>\n", is_lscope);
	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&unused=1&match=L&qf=1&n=Files+Containing+Unused+File-scoped+Writable+Identifiers\">Files containing unused file-scoped writable identifiers</a>\n", is_cscope);
		fprintf(of, "<li> <a href=\"xfquery.html?writable=1&c%d=%d&n%d=0&match=L&fre=%%5C.%%5BcC%%5D%%24&n=Writable+.c+Files+Without+Any+Statments&qf=1\">Writable .c files without any statements</a>\n", em_nstatement, Query::ec_eq, em_nstatement);
		fprintf(of, "<li> <a href=\"xfquery.html?writable=1&c%d=%d&n%d=0&match=L&qf=1&n=Writable+Files+Containing+Strings\">Writable files containing strings</a>\n", em_nstring, Query::ec_gt, em_nstring);
		fprintf(of, "<li> <a href=\"xfquery.html?writable=1&c%d=%d&n%d=0&match=L&fre=%%5C.%%5BhH%%5D%%24&n=Writable+.h+Files+With+%%23include+directives&qf=1\">Writable .h files with #include directives</a>\n", em_nincfile, Query::ec_gt, em_nincfile);
		fprintf(of, "<li> <a href=\"fquery.html\">Specify new file query</a>\n"
		"</ul>\n"
		"<h2>Identifiers</h2>\n"
		"<ul>\n"
		"<li> <a href=\"idmetrics.html\">Identifier Metrics</a>\n"
		);
	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&match=Y&qi=1&n=All+Identifiers\">All identifiers</a>\n", is_readonly);
	fprintf(of, "<li> <a href=\"xiquery.html?a%d=1&match=Y&qi=1&n=Read-only+Identifiers\">Read-only identifiers</a>\n", is_readonly);
	fputs("<li> <a href=\"xiquery.html?writable=1&match=Y&qi=1&n=Writable+Identifiers\">Writable identifiers</a>\n"
		"<li> <a href=\"xiquery.html?writable=1&xfile=1&match=L&qi=1&n=File-spanning+Writable+Identifiers\">File-spanning writable identifiers</a>\n", of);
	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&unused=1&match=L&qi=1&n=Unused+Project-scoped+Writable+Identifiers\">Unused project-scoped writable identifiers</a>\n", is_lscope);
	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&unused=1&match=L&qi=1&n=Unused+File-scoped+Writable+Identifiers\">Unused file-scoped writable identifiers</a>\n", is_cscope);
	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&unused=1&match=L&qi=1&n=Unused+Writable+Macros\">Unused writable macros</a>\n", is_macro);
	// xfile is implicitly 0
	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&a%d=1&match=T&ire=&fre=&n=Writable+identifiers+that+should+be+made+static&qi=1\">Writable identifiers that should be made static</a>\n", is_ordinary, is_lscope);
	fprintf(of,
		"<li> <a href=\"iquery.html\">Specify new identifier query</a>\n"
		"</ul>"
		"<h2>Functions</h2>\n"
		"<ul>\n"
		"<li> <a href=\"cgraph.html\">Call graph</a>\n"
		"<li> <a href=\"funquery.html\">Specify new function query</a>\n"
		"</ul>\n"
		"<h2>Operations</h2>"
		"<ul>\n"
		"<li> <a href=\"options.html\">Global options</a>\n"
		"<li> <a href=\"sproject.html\">Select active project</a>\n"
		"<li> <a href=\"sexit.html\">Exit - saving changes</a>\n"
		"<li> <a href=\"qexit.html\">Exit - ignore changes</a>\n"
		"</ul>");
	html_tail(of);
}

void
file_page(FILE *of, void *p)
{
	ostringstream fname;
	int id;
	if (!swill_getargs("i(id)", &id)) {
		fprintf(of, "Missing value");
		return;
	}
	Fileid i(id);
	const string &pathname = i.get_path();
	fname << i.get_id();
	html_head(of, "file", string("File: ") + html(pathname));
	fprintf(of, "<h2>Metrics</h2><ul>\n");
	fprintf(of, "<li> Read-only: %s", i.get_readonly() ? "Yes" : "No");
	for (int j = 0; j < metric_max; j++)
		fprintf(of, "\n<li> %s: %d", Metrics::name(j).c_str(), i.metrics().get_metric(j));
	fprintf(of, "\n<li> Used in project(s): \n<ul>");
	for (Attributes::size_type j = attr_end; j < Attributes::get_num_attributes(); j++)
		if (i.get_attribute(j))
			fprintf(of, "<li>%s\n", Project::get_projname(j).c_str());
	fprintf(of, "</ul>\n</ul><h2>Listings</h2><ul>\n<li> <a href=\"src.html?id=%s\">Source code</a>\n", fname.str().c_str());
	fprintf(of, "<li> <a href=\"qsrc.html?id=%s&match=Y&writable=1&a%d=1&n=Source+Code+With+Identifier+Hyperlinks\">Source code with identifier hyperlinks</a>\n", fname.str().c_str(), is_readonly);
	fprintf(of, "<li> <a href=\"qsrc.html?id=%s&match=L&writable=1&a%d=1&n=Source+Code+With+Hyperlinks+to+Project-global+Writable+Identifiers\">Source code with hyperlinks to project-global writable identifiers</a>\n", fname.str().c_str(), is_lscope);
	fprintf(of, "</ul>\n<h2>Include Files</h2><ul>\n");
	fprintf(of, "<li> <a href=\"qinc.html?id=%s&direct=1&writable=1&includes=1&n=Directly+Included+Writable+Files\">Writable files that this file directly includes</a>\n", fname.str().c_str());
	fprintf(of, "<li> <a href=\"qinc.html?id=%s&includes=1&n=All+Included+Files\">All files that this file includes</a>\n", fname.str().c_str());
	fprintf(of, "<li> <a href=\"qinc.html?id=%s&direct=1&unused=1&includes=1&n=Unused+Directly+Included+Files\">Unused directly included files</a>\n", fname.str().c_str());
	fprintf(of, "<li> <a href=\"qinc.html?id=%s&n=Files+Including+the+File\">Files including this file</a>\n", fname.str().c_str());
	fprintf(of, "</ul>\n");
	html_tail(of);
}

void
source_page(FILE *of, void *p)
{
	int id;
	if (!swill_getargs("i(id)", &id)) {
		fprintf(of, "Missing value");
		return;
	}
	Fileid i(id);
	const string &pathname = i.get_path();
	html_head(of, "src", string("Source: ") + html(pathname));
	file_hypertext(of, i, false);
	html_tail(of);
}

void
query_source_page(FILE *of, void *p)
{
	int id;
	if (!swill_getargs("i(id)", &id)) {
		fprintf(of, "Missing value");
		return;
	}
	Fileid i(id);
	const string &pathname = i.get_path();
	char *qname = swill_getvar("n");
	if (qname && *qname)
		html_head(of, "qsrc", string(qname) + ": " + html(pathname));
	else
		html_head(of, "qsrc", string("Source with queried identifiers marked: ") + html(pathname));
	fputs("<p>(Use the tab key to move to each marked identifier.)<p>", of);
	file_hypertext(of, i, true);
	html_tail(of);
}

void
query_include_page(FILE *of, void *p)
{
	int id;
	if (!swill_getargs("i(id)", &id)) {
		fprintf(of, "Missing value");
		return;
	}
	Fileid f(id);
	const string &pathname = f.get_path();
	char *qname = swill_getvar("n");
	if (qname && *qname)
		html_head(of, "qinc", string(qname) + ": " + html(pathname));
	else
		html_head(of, "qinc", string("Include File Query: ") + html(pathname));
	bool writable = !!swill_getvar("writable");
	bool direct = !!swill_getvar("direct");
	bool unused = !!swill_getvar("unused");
	bool includes = !!swill_getvar("includes");
	const FileIncMap &m = includes ? f.get_includes() : f.get_includers();
	html_file_begin(of);
	for (FileIncMap::const_iterator i = m.begin(); i != m.end(); i++) {
		Fileid f2 = (*i).first;
		const IncDetails &id = (*i).second;
		if ((!writable || !f2.get_readonly()) &&
		    (!direct || id.is_directly_included()) &&
		    (!unused || !id.is_required())) {
			html_file(of, f2);
			if (id.is_directly_included()) {
				fprintf(of, " - line ");
				const set <int> &lines = id.include_line_numbers();
				for (set <int>::const_iterator j = lines.begin(); j != lines.end(); j++)
					fprintf(of, "%d ", *j);
			}
			if (!id.is_required())
				fprintf(of, " (not required)");
		}
	}
	html_file_end(of);
	fputs("</ul>\n", of);
	html_tail(of);
}

static bool must_exit = false;

void
write_quit_page(FILE *of, void *p)
{
#ifndef COMMERCIAL
	if (!local_access(of))
		return;
#endif
	// Determine files we need to process
	IFSet process;
	cout << "Examing identifiers for replacement\n";
	for (IdProp::iterator i = ids.begin(); i != ids.end(); i++) {
		progress(i, ids);
		if ((*i).second.get_replaced()) {
			Eclass *e = (*i).first;
			IFSet ifiles = e->sorted_files();
			process.insert(ifiles.begin(), ifiles.end());
		}
	}
	cout << '\n';
	// Now do the replacements
	int replacements = 0;
	cout << "Processing files\n";
	for (IFSet::const_iterator i = process.begin(); i != process.end(); i++) {
		cout << "Processing file " << (*i).get_path() << "\n";
		replacements += file_replace(*i);
	}
	html_head(of, "quit", "CScout exiting");
	fprintf(of, "A total of %d replacements were made in %d files.", replacements, process.size());
	fprintf(of, "<p>Bye...</body></html>");
	must_exit = true;
}

void
quit_page(FILE *of, void *p)
{
#ifndef COMMERCIAL
	if (!local_access(of))
		return;
#endif
	html_head(of, "quit", "CScout exiting");
	fprintf(of, "No changes were saved.");
	fprintf(of, "<p>Bye...</body></html>");
	must_exit = true;
}

#ifdef COMMERCIAL
/*
 * Parse the access control list cscout_acl.
 * The ACL is searched in three different directories:
 * .cscout, $CSCOUT_HOME, and $HOME/.cscout
 */
static void
parse_acl()
{

	ifstream in;
	string ad, host;
	vector <string> dirs;
	dirs.push_back(".cscout");
	if (getenv("CSCOUT_HOME"))
		dirs.push_back(getenv("CSCOUT_HOME"));
	if (getenv("HOME"))
		dirs.push_back(string(getenv("HOME")) + "/.cscout");
	vector <string>::const_iterator i;
	string fname;

	for (i = dirs.begin(); i != dirs.end(); i++) {
		fname = *i + "/cscout_acl";
		in.open(fname.c_str());
		if (in.fail())
			in.clear();
		else {
			cout << "Parsing ACL from " << fname << "\n";
			for (;;) {
				in >> ad;
				if (in.eof())
					break;
				in >> host;
				if (ad == "A") {
					cout << "Allow from IP address " << host << "\n";
					swill_allow(host.c_str());
				} else if (ad == "D") {
					cout << "Deny from IP address " << host << "\n";
					swill_deny(host.c_str());
				} else
					cout << "Bad ACL specification " << ad << " " << host << "\n";
			}
			break;
		}
	}
	if (i == dirs.end()) {
		cout << "No ACL found.  Only localhost access will be allowed.\n";
		swill_allow("127.0.0.1");
	}
}
#endif

// Process the input as a C preprocessor
// Fchar should already have its input set
static int
simple_cpp()
{
	for (;;) {
		Pdtoken t;

		t.getnext();
		if (t.get_code() == EOF)
			break;
		if (t.get_code() == STRING_LITERAL)
			cout << "\"";
		else if (t.get_code() == CHAR_LITERAL)
			cout << "'";

		cout << t.get_val();

		if (t.get_code() == STRING_LITERAL)
			cout << "\"";
		else if (t.get_code() == CHAR_LITERAL)
			cout << "'";
	}
	return(0);
}

// Included file site information
// See warning_report
class SiteInfo {
private:
	bool required;		// True if this site contains at least one required include file
	set <Fileid> files;	// Files included here
public:
	SiteInfo(bool r, Fileid f) : required(r) {
		files.insert(f);
	}
	void update(bool r, Fileid f) {
		required |= r;
		files.insert(f);
	}
	const set <Fileid> & get_files() const { return files; }
	bool is_required() const { return required; }
};

// Generate a warning report
static void
warning_report()
{
	struct {
		char *message;
		char *query;
	} reports[] = {
		{ "unused project scoped writable identifier",
		  "L:writable:unused:pscope" },
		{ "unused file scoped writable identifier",
		  "L:writable:unused:fscope" },
		{ "unused writable macro",
		  "L:writable:unused:macro" },
		{ "writable identifier should be made static",
		  "T:writable:obj:pscope" }, // xfile is implicitly 0
	};

	// Generate identifier warnings
	for (unsigned i = 0; i < sizeof(reports) / sizeof(reports[0]); i++) {
		IdQuery query(reports[i].query);

		assert(query.is_valid());
		for (IdProp::iterator j = ids.begin(); j != ids.end(); j++) {
			if (!query.eval(*j))
				continue;
			const Tokid t = *((*j).first->get_members().begin());
			const string &id = (*j).second.get_id();
			cerr << t.get_path() << ':' <<
				t.get_fileid().line_number(t.get_streampos()) << ": " <<
				id << ": " << reports[i].message << '\n';
		}
	}

	/*
	 * Generate unneeded include file warnings
	 * A given include directive can include different files on different
	 * compilations (through different include paths or macros)
	 * Therefore maintain a map for include directive site information:
	 */

	typedef map <int, SiteInfo> Sites;
	Sites include_sites;

	for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++) {
		if ((*i).get_readonly() || !(*i).compilation_unit() || *i == input_file_id)
			continue;
		const FileIncMap &m = (*i).get_includes();
		// Find the status of our include sites
		include_sites.clear();
		for (FileIncMap::const_iterator j = m.begin(); j != m.end(); j++) {
			Fileid f2 = (*j).first;
			const IncDetails &id = (*j).second;
			if (!id.is_directly_included())
				continue;
			const set <int> &lines = id.include_line_numbers();
			for (set <int>::const_iterator k = lines.begin(); k != lines.end(); k++) {
				Sites::iterator si = include_sites.find(*k);
				if (si == include_sites.end())
					include_sites.insert(Sites::value_type(*k, SiteInfo(id.is_required(), f2)));
				else
					(*si).second.update(id.is_required(), f2);
			}
		}
		// And report those containing unused files
		Sites::const_iterator si;
		for (si = include_sites.begin(); si != include_sites.end(); si++)
			if (!(*si).second.is_required()) {
				const set <Fileid> &sf = (*si).second.get_files();
				int line = (*si).first;
				for (set <Fileid>::const_iterator fi = sf.begin(); fi != sf.end(); fi++)
					cerr << (*i).get_path() << ':' <<
						line << ": unused included file " <<
						(*fi).get_path() << '\n';
			}
	}
}

// Report usage information and exit
static void
usage(char *fname)
{
	cerr << "usage: " << fname << " [-cEruv] [-p port] [-m spec] file\n"
		"\t-c\tProcess the file and exit\n"
		"\t-E\tPrint preprocessed results on standard output and exit\n"
		"\t\t(the workspace file must have also been processed with -E)\n"
		"\t-p port\tSpecify TCP port for serving the CScout web pages\n"
		"\t\t(the port number must be in the range 1024-32767)\n"
		"\t-r\tGenerate an identifier and include file warning report\n"
		"\t-v\tDisplay version and copyright information and exit\n"
		"\t-m spec\tSpecify identifiers to monitor (unsound)\n";
	exit(1);
}

int
main(int argc, char *argv[])
{
	Pdtoken t;
	char *motd;
	int c;

	Debug::db_read();

#ifdef COMMERCIAL
	// Decode name of licensee
	#include "lname.c"
	char lkey[] = LKEY;
	cscout_des_init(0);
	cscout_des_set_key(lkey);
	for (int i = 0; i < sizeof(licensee) / 8; i++)
		cscout_des_decode(licensee + i * 8);
	cscout_des_done();
#endif


	while ((c = getopt(argc, argv, "crvEp:m:")) != EOF)
		switch (c) {
		case 'E':
			preprocess = true;
			break;
		case 'c':
			compile_only = true;
			break;
		case 'p':
			if (!optarg)
				usage(argv[0]);
			portno = atoi(optarg);
			if (portno < 1024 || portno > 32767)
				usage(argv[0]);
			break;
		case 'm':
			if (!optarg)
				usage(argv[0]);
			monitor = IdQuery(optarg);
			break;
		case 'r':
			report = true;
			break;
		case 'v':
			cerr << "CScout version " <<
			Version::get_revision() << " - " <<
			Version::get_date() << "\n\n"
			// 80 column terminal width---------------------------------------------------
			"(C) Copyright 2003 Diomidis Spinelllis, Athens, Greece.\n\n"
#ifdef COMMERCIAL
			"Commercial version.  All rights reserved.\n"
			"Licensee: " << licensee << ".\n";
#else /* !COMMERCIAL */
			"Unsupported version.  Can be used and distributed under the terms of the\n"
			"CScout Public License available in the CScout documentation and online at\n"
			"http://www.spinellis.gr/cscout/doc/license.html\n";
#endif
			exit(0);
		case '?':
			usage(argv[0]);
		}


	// We require exactly one argument
	if (argv[optind] == NULL || argv[optind + 1] != NULL)
		usage(argv[0]);

	if (preprocess) {
		Fchar::set_input(argv[optind]);
		return simple_cpp();
	}

	if (!compile_only && !swill_init(portno)) {
		cerr << "Couldn't initialize our web server on port " << portno << "\n";
		exit(1);
	}

	license_init();

#ifdef COMMERCIAL
	parse_acl();
#endif

	// Pass 1: process master file loop
	Fchar::set_input(argv[optind]);
	do
		t.getnext();
	while (t.get_code() != EOF);

	input_file_id = Fileid(argv[optind]);

	if (compile_only && !report)
		return 0;

	// Pass 2: Create web pages
	files = Fileid::files(true);

	if (!compile_only) {
		swill_handle("sproject.html", select_project_page, 0);
		swill_handle("options.html", options_page, 0);
		swill_handle("soptions.html", set_options_page, 0);
		swill_handle("sexit.html", write_quit_page, 0);
		swill_handle("qexit.html", quit_page, 0);
	}

	// Populate the EC identifier member
	for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++)
		/* bool has_unused = */ file_analyze(*i);

	// Set xfile and  metrics for each identifier
	cout << "Processing identifiers\n";
	for (IdProp::iterator i = ids.begin(); i != ids.end(); i++) {
		progress(i, ids);
		Eclass *e = (*i).first;
		IFSet ifiles = e->sorted_files();
		(*i).second.set_xfile(ifiles.size() > 1);
		// Update metrics
		id_msum.add_unique_id(e);
	}
	cout << '\n';

	// Update fle metrics
	file_msum.summarize_files();
	if (DP())
		cout << "Size " << file_msum.get_total(em_nchar) << "\n";

#ifdef COMMERCIAL
	motd = license_check(licensee, url(Version::get_revision()).c_str(), file_msum.get_total(em_nchar));
#else
	/*
	 * Send the metrics
	 * up to 10 project names
	 * up 50 cross-file identifiers
	 */
	ostringstream mstring;
	mstring << file_msum;
	mstring << id_msum;
	mstring << "\nxids: ";
	int count = 0;
	for (IdProp::iterator i = ids.begin(); i != ids.end(); i++) {
		if ((*i).second.get_xfile() == true)
			mstring << (*i).second.get_id() << ' ';
		if (count++ > 100)
			break;
	}
	mstring << "\nprojnames: ";
	count = 0;
	for (Attributes::size_type j = attr_end; j < Attributes::get_num_attributes(); j++) {
		mstring << Project::get_projname(j) << ' ';
		if (count++ > 10)
			break;
	}
	mstring << "\n";
	motd = license_check(mstring.str().c_str(), Version::get_revision().c_str(), file_msum.get_total(em_nchar));
#endif

	if ((must_exit = (CORRECTION_FACTOR - license_offset != 0))) {
#ifndef PRODUCTION
		cout << "**********Unable to obtain correct license*********\n";
		cout << "license_offset = " << license_offset << "\n";
#endif
	}

	if (!compile_only) {
		swill_handle("src.html", source_page, NULL);
		swill_handle("qsrc.html", query_source_page, NULL);
		swill_handle("file.html", file_page, NULL);

		// Identifier query and execution
		swill_handle("iquery.html", iquery_page, NULL);
		swill_handle("xiquery.html", xiquery_page, NULL);
		// File query and execution
		swill_handle("fquery.html", fquery_page, NULL);
		swill_handle("xfquery.html", xfquery_page, NULL);
		swill_handle("qinc.html", query_include_page, NULL);

		// Function query and execution
		swill_handle("funquery.html", funquery_page, NULL);
		swill_handle("xfunquery.html", xfunquery_page, NULL);

		swill_handle("id.html", identifier_page, NULL);
		swill_handle("fun.html", function_page, NULL);
		swill_handle("funlist.html", funlist_page, NULL);
		swill_handle("fmetrics.html", file_metrics_page, NULL);
		swill_handle("idmetrics.html", id_metrics_page, NULL);
		swill_handle("cgraph.html", cgraph_page, NULL);
		swill_handle("setproj.html", set_project_page, NULL);
		swill_handle("index.html", (void (*)(FILE *, void *))((char *)index_page - CORRECTION_FACTOR + license_offset), 0);
	}

	if (motd)
		cout << motd << "\n";
	if (report && !must_exit)
		warning_report();
	if (compile_only)
		return 0;
	if (DP())
		cout  << "Tokid EC map size is " << Tokid::map_size() << "\n";
	// Serve web pages
	if (!must_exit)
		cout << "We are now ready to serve you at http://localhost:" << portno << "\n";
	while (!must_exit)
		swill_serve();

#ifdef NODE_USE_PROFILE
	cout << "Type node count = " << Type_node::get_count() << "\n";
#endif
	return (0);
}


// Clear equivalence classes that do not
// satisfy the monitoring criteria
void
garbage_collect(Fileid root)
{
	vector <Fileid> files(Fileid::files(false));
	set <Fileid> touched_files;

	int count = 0;
	int sum = 0;

	root.set_compilation_unit(true);
	for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++) {
		Fileid fi = (*i);

		if (fi.garbage_collected())
			continue;

		fi.set_required(false);	// Mark the file as not being required
		touched_files.insert(*i);

		if (!monitor.is_valid()) {
			fi.set_gc(true);	// Mark the file as garbage collected
			continue;
		}

		const string &fname = fi.get_path();
		ifstream in;

		in.open(fname.c_str(), ios::binary);
		if (in.fail()) {
			perror(fname.c_str());
			exit(1);
		}
		// Go through the file character by character
		for (;;) {
			Tokid ti;
			int val;

			ti = Tokid(fi, in.tellg());
			if ((val = in.get()) == EOF)
				break;
			mapTokidEclass::iterator ei = ti.find_ec();
			if (ei != ti.end_ec()) {
				sum++;
				Eclass *ec = (*ei).second;
				IdPropElem ec_id(ec, Identifier());
				if (!monitor.eval(ec_id)) {
					count++;
					ec->remove_from_tokid_map();
					delete ec;
				}
			}
		}
		in.close();
		fi.set_gc(true);	// Mark the file as garbage collected
	}
	if (DP())
		cout << "Garbage collected " << count << " out of " << sum << " ECs\n";

	// Monitor dependencies
	set <Fileid> required_files;

	// Recursively mark all the files containing definitions for us
	Fdep::mark_required(root);
	// Store them in a set to calculate set difference
	for (set <Fileid>::const_iterator i = touched_files.begin(); i != touched_files.end(); i++)
		if (*i != root && *i != input_file_id)
			root.includes(*i, /* directly included = */ false, (*i).required());
	Fdep::reset();

	return;
}
