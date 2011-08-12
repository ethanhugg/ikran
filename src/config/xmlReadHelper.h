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

#ifndef XML_READ_HELPER_H
#define XML_READ_HELPER_H

#include <iostream>
#include <sstream>

#include <string>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

class XmlNode
{
public:
	xmlDocPtr document_;
	xmlNodePtr node_;

private:
	xmlNodePtr findNode(xmlNodePtr node) const;
	xmlNodePtr findNodeByName(xmlNodePtr node, const xmlChar* name) const;

public:
	XmlNode(xmlDocPtr docPtr);
	XmlNode(xmlDocPtr d, xmlNodePtr c);
    operator bool() const { return node_ != NULL; }

	XmlNode getParent() const;

	XmlNode firstChild() const;
	XmlNode nextSibling() const;

	int getChildCountByName(const char* name) const;
	XmlNode getChildByName(const char* name) const;
	XmlNode getNextSiblingByName() const;
    // Hierarchy is in the form of nodeX/nodeY/nodeZ
    XmlNode getChildByHierarchy(const char* hierarchy) const;

	bool isName(const char* name) const;
	const char* getName() const;
	int getType() const;

	bool hasAttribute(const char* attr) const;
	std::string getAttribute(const char* attr) const;
	std::string getString() const;

    std::string dumpElementDetails();
};

template <class T>
inline bool getAttribute(XmlNode& node, const char* attr, T& value) 
{
	if (node && node.hasAttribute(attr)) 
	{
		std::istringstream s(node.getAttribute(attr));
		s >> value;
		return true;
	}
	return false;
}

template <>
inline bool getAttribute(XmlNode& node, const char* attr, std::string& value)
{
    if ((node.node_ != NULL) && (node.hasAttribute(attr)))
	{
		value = node.getAttribute(attr);
		return true;
	}
	return false;
}

#endif // XML_READ_H
