/* 
 * (C) Copyright 2002 Diomidis Spinellis.
 *
 * For documentation read the corresponding .h file
 *
 * $Id: attr.cpp,v 1.6 2002/12/25 15:44:20 dds Exp $
 */

#include <map>
#include <string>
#include <deque>
#include <vector>
#include <stack>
#include <iterator>
#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <cassert>

#include "cpp.h"
#include "debug.h"
#include "metrics.h"
#include "attr.h"
#include "fileid.h"


// Leave space for a single project-attribute
int Attributes::size = attr_max + 1;

int Project::projid = attr_max - 1;
// Maps between ids and names
map<string, int> Project::projids;
vector<string> Project::projnames(attr_max);


void
Project::set_current_project(const string &name)
{
	map<string, int>::const_iterator p;
	if ((p = projids.find(name)) == projids.end()) {
		// Add new project
		projid++;
		projnames.push_back(name);
		projids[name] = projid;
		if (Attributes::get_num_attributes() < projnames.size())
			Attributes::add_attribute();
	} else {
		projid = (*p).second;
	}
}