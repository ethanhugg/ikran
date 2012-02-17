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

#pragma once

#include <queue>
#include <string>

#include "SharedPtr.h"
#include "base/threading/simple_thread.h"


class WorkItem
{
public:
	virtual ~WorkItem() {};
	virtual void run() {};

protected:
	WorkItem() {};
};

DECLARE_PTR(WorkItem);

/* WorkerThread
 *
 * A simple worker thread.  Enqueue() adds a work item to the queue, transferring ownership
 * of the WorkItem to WorkerThread - WorkerThread will delete it after it has invoked it.
 * TODO: Still using a raw pointer for this - is there any advantage in a smart pointer for this purpose?
 *
 * When WorkerThread is destroyed it will complete all queued work items before the destructor completes.
 * This blocks the thread calling the destructor until the work items are done.
 *
 * The destructor ensures that join() is called.  However, join() is provided as an explicit method call
 * to allow the owner to ensure that the worker thread is stopped before other cleanup kicks in.  (This
 * is useful where the WorkerThread is a data member in a class and the cleanup sequence of the data
 * members is not sufficiently obvious.)
 *
 * To create a work item, simply:
 *  - Subclass WorkItem
 *  - Override run()
 *  - Add a constructor with args if you want to pass in any values, and use it to store them in
 *    private members until run() actually gets invoked.
 *  - You can make it an inner class if run() needs access to private members of the class creating
 *    the work item.
 */
class WorkerThreadData;

class WorkerThread : public base::SimpleThread {
public:
	WorkerThread();
	WorkerThread(const std::string& name);
	virtual ~WorkerThread();
	void enqueue(WorkItemPtr);
	void enqueue(WorkItem*);
	void join();  // join() provided to allow callers to ensure the thread is stopped before destructors get called en masse.
	void Start() {};
	void Join() {};
	virtual void Run() {};
	void ThreadMain() {};

private:
	WorkerThreadData* m_data;

	// Status flags
	bool m_stopping;	// Stop requested
	bool m_started;		// run() entered
	bool m_complete;	// run() exiting

protected:
	friend class WorkerThreadData;
	void run();								 // Does all the work
};

