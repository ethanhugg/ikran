/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Cisco Systems SIP Stack.
 *
 * The Initial Developer of the Original Code is
 * Cisco Systems (CSCO).
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Enda Mannion <emannion@cisco.com>
 *  Suhas Nandakumar <snandaku@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <iostream>
#include <sstream>

#include <string>
#include <cstring>

#include <map>
#include <vector>
#include <algorithm>

using namespace std;

#include "xmlReadHelper.h"

/****************************************************************************/
XmlNode::XmlNode(xmlDocPtr docPtr) : 
	document_(docPtr),
	node_(xmlDocGetRootElement(docPtr)) 
{
}

XmlNode::XmlNode(xmlDocPtr d, xmlNodePtr c) : 
	document_(d),
	node_(c)
{
}

XmlNode XmlNode::getParent() const
{
	return XmlNode(document_, node_->parent);
}

XmlNode XmlNode::firstChild() const
{
	xmlNodePtr child = findNode(node_->xmlChildrenNode);
	return XmlNode(document_, child);
}

XmlNode XmlNode::nextSibling() const
{
	xmlNodePtr child = findNode(node_->next);
	return XmlNode(document_, child);
}

xmlNodePtr XmlNode::findNode(xmlNodePtr elem) const
{
	while (elem)
	{
		if (elem->type == XML_ELEMENT_NODE)
		{
			break;
		}

		elem = elem->next;
	}

	// node may be NULL
	return elem;
}

int XmlNode::getChildCountByName(const char* nameSigned) const
{
	const xmlChar* name = (const xmlChar *)nameSigned;
	xmlNodePtr elem = node_->xmlChildrenNode;
	int count = 0;
	while (elem)
	{
		if ((elem->type == XML_ELEMENT_NODE)
				&& !xmlStrcmp(elem->name, name))
		{
			++count;
		}

		elem = elem->next;
	}

	return count;
}

xmlNodePtr XmlNode::findNodeByName(xmlNodePtr elem, const xmlChar* name) const
{
	while (elem)
	{
		if (elem->type == XML_ELEMENT_NODE)
		{
			if (!xmlStrcmp(elem->name, name))
			{
				// found first
				break;
			}
		}

		elem = elem->next;
	}

	// elem may be NULL
	return elem;
}

XmlNode XmlNode::getChildByName(const char* nameSigned) const
{
	const xmlChar* name = (const xmlChar *)nameSigned;

    if (node_ == NULL) return XmlNode(document_, NULL);

	xmlNodePtr child = findNodeByName(node_->xmlChildrenNode, name);
	return XmlNode(document_, child);
}

XmlNode XmlNode::getNextSiblingByName() const
{
    if (node_ == NULL) return XmlNode(document_, NULL);

	xmlNodePtr child = findNodeByName(node_->next, node_->name);
	return XmlNode(document_, child);
}

XmlNode XmlNode::getChildByHierarchy(const char* hierarchy) const
{
    if ((hierarchy == NULL) || (strlen(hierarchy) == 0)) return XmlNode(document_, NULL);

    std::string token;  
    std::istringstream stream(hierarchy);  
    
    if(!std::getline(stream, token, '/'))
    {
        return XmlNode(document_, NULL);
    }

    // Initial '/'
    XmlNode node = getChildByName(token.c_str());
    if(!node)
    {
        return node;
    }

    // Subsequent '/'
    while(std::getline(stream, token, '/'))  
    {     
        node = node.getChildByName(token.c_str());
        if(!node)
        {
            break;
        }
    }

    return node;
}

const char* XmlNode::getName() const { if (node_ == NULL) return NULL; return (const char *)node_->name; }

int XmlNode::getType() const { return node_->type; }

bool XmlNode::hasAttribute (const char* attr) const
{
    if (node_ == NULL) return false;

    return (xmlHasProp(node_, (const xmlChar *) attr) != NULL);
}

std::string XmlNode::getAttribute (const char* attr) const
{
    xmlChar * pAttrib = NULL;

    if ((node_ == NULL) || (attr == NULL)) return "";

    return (((pAttrib = xmlGetProp(node_, (const xmlChar *) attr)) == NULL) ? "" : (const char * ) pAttrib);
}

bool XmlNode::isName(const char* name) const
{
    if ((node_ == NULL) || (node_->name == NULL) || (name == NULL)) return false;
	const int r = xmlStrcmp(node_->name, (const xmlChar *)name);
	return (0 == r);
}

std::string XmlNode::getString() const
{
    if ((document_ == NULL) || (node_ == NULL) || (node_->xmlChildrenNode == NULL)) return "";

	xmlChar * text = xmlNodeListGetString(document_, node_->xmlChildrenNode, true);

    if (text == NULL) return "";

	std::string s((const char*)text);
	xmlFree(text);
	return s;
}

std::string XmlNode::dumpElementDetails()
{
    if ((document_ == NULL) || (node_ == NULL) || (node_->type != XML_ELEMENT_NODE)) return "";

    xmlBufferPtr xmlBuffer = xmlBufferCreate();

    std::string str;
    if (xmlNodeDump(xmlBuffer, document_, node_, 1, 0) != -1)
    {
        str = (const char *) xmlBuffer->content;
    }

    xmlBufferFree(xmlBuffer);

    return str;
}
