/*
 * (C) Copyright 2002-2024 Diomidis Spinellis
 *
 * This file is part of CScout.
 *
 * CScout is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CScout is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CScout.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * For documentation read the corresponding .h file
 *
 */

#include <iostream>
#include <map>
#include <string>
#include <deque>
#include <fstream>
#include <stack>
#include <set>
#include <vector>
#include <sstream>		// ostringstream
#include <list>
#include <cmath>		// log
#include <errno.h>

#include "cpp.h"
#include "debug.h"
#include "error.h"
#include "attr.h"
#include "metrics.h"
#include "funmetrics.h"
#include "filemetrics.h"
#include "fileid.h"
#include "tokid.h"
#include "tokmap.h"
#include "eclass.h"
#include "fchar.h"
#include "token.h"
#include "call.h"
#include "parse.tab.h"
#include "tokid.h"
#include "ctoken.h"
#include "ptoken.h"
#include "pltoken.h"


vector <MetricDetails>
FunMetrics::metric_details_values()
{
	vector <MetricDetails> v(metric_max);

	// First elements come from Metrics
	copy(Metrics::get_metric_details_vector().begin(),
	    Metrics::get_metric_details_vector().end(), v.begin());

	// During processing (once based on processed)
					// pre-cpp, post-cpp, file
	v[em_ngnsoc]	= MetricDetails(0, 1, 0, "NGNSOC",	"Number of global namespace occupants at function's top");
	v[em_nmparam]	= MetricDetails(1, 0, 0, "NMPARAM",	"Number of parameters (for macros)");
	v[em_nfparam]	= MetricDetails(0, 1, 0, "NFPARAM",	"Number of parameters (for functions)");
	v[em_nneparam]	= MetricDetails(1, 0, 0, "NEPARAM",	"Number of passed non-expression macro parameters");
	// Metrics dynamically derived
	v[em_fanin]	= MetricDetails(1, 1, 0, "FANIN",	"Fan-in (number of calling functions)");
	v[em_fanout]	= MetricDetails(1, 1, 0, "FANOUT",	"Fan-out (number of called functions)");
	v[em_ccycl1]	= MetricDetails(1, 1, 0, "CCYCL1",	"Cyclomatic complexity (control statements)");
	v[em_ccycl2]	= MetricDetails(1, 1, 0, "CCYCL2",	"Extended cyclomatic complexity (includes branching operators)");
	v[em_ccycl3]	= MetricDetails(1, 1, 0, "CCYCL3",	"Maximum cyclomatic complexity (includes branching operators and all switch branches)");
	// Dynamically derived metrics after this point are hardcoded to be stored in the database as REAL
	v[em_cstruc]	= MetricDetails(1, 1, 0, "CSTRUC",	"Structure complexity (Henry and Kafura)");	// REAL
	v[em_chal]	= MetricDetails(1, 1, 0, "CHAL",	"Halstead volume");				// REAL
	v[em_iflow]	= MetricDetails(1, 1, 0, "IFLOW",	"Information flow metric (Henry and Selig)");	// REAL
	return v;
};

double
FunMetrics::get_metric(int n) const
{
	switch (n) {
	// Most metrics are simply looked up
	default:
		return Metrics::get_metric(n);
	// A few are calculated dynamically
	case em_fanin:	// Fan-in (number of calling functions)
		return call->get_num_caller();
	case em_fanout:	// Fan-out (number of called functions)
		return call->get_num_call();
	case em_ccycl1:	// Cyclomatic complexity (control statements)
		return (get_metric(em_nif) + get_metric(em_nswitch) +
		    get_metric(em_nfor) + get_metric(em_nwhile) + get_metric(em_ndo) + 1);
	case em_ccycl2:	// Extended cyclomatic complexity (including branching operators)
		return get_metric(em_ccycl1) + get_metric(em_ncc2op);
	case em_ccycl3:	// Cyclomatic complexity (including switch branches)
		return get_metric(em_ccycl2) - get_metric(em_nswitch) + get_metric(em_ncase);
	case em_cstruc:	// Structure complexity (Henry and Kafura)
#define sqr(x) ((x) * (x))
		return sqr((double)get_metric(em_fanin) * (double)get_metric(em_fanout));
	case em_iflow:	// Information flow metric
		return get_metric(em_cstruc) * get_metric(em_ccycl1);
	case em_chal:	// Halstead volume
#define log2(x) (log(x) / log(2.))
		// We consider numeric constants and character literals to be unique operands
		double logarg = get_metric(em_nuop) +
		        get_metric(em_nuid) +
			get_metric(em_nnconst) +
			get_metric(em_nclit);
		if (logarg == 0)
			return 0;
		else
			return (get_metric(em_nop) +
				get_metric(em_nid) +
				get_metric(em_nnconst) +
				get_metric(em_nclit)) *
			    log2(logarg);
	}
}

// Summarize the operators collected by process_token
void
FunMetrics::summarize_operators()
{
	if (processed)
		return;
	count[em_nuop] = operators.size();
	operators.clear();
}

// Global metrics
FunMetricsSummary fun_msum;

// Create function summary
void
FunMetricsSummary::summarize_functions()
{
	Call::const_fmap_iterator_type i;
	for (i = Call::fbegin(); i != Call::fend(); i++)
		if (i->second->is_defined()) {
			val.total.add(*(i->second), plus<double>());
			val.min.add(*(i->second), get_min());
			val.max.add(*(i->second), get_max());
		}
}

ostream&
operator<<(ostream& o, const FunMetricsSummary &ms)
{
	o << ms.val;
	return o;
}
