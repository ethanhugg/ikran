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


#include "WorkerThread.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"


void WorkerThread::enqueue(WorkItem* workItem)
{
	WorkItemPtr p(workItem);
	enqueue(p);
}


class WorkerThreadData
{
public:
	std::queue<WorkItemPtr> m_workQueue;
	std::string m_name;

	base::Lock* m_mutex;
	base::ConditionVariable* m_workerDone;
	base::ConditionVariable* m_workAdded;

	WorkerThreadData();
};

WorkerThreadData::WorkerThreadData() {
	m_mutex = new base::Lock();
	m_workerDone = new base::ConditionVariable(m_mutex);
	m_workAdded = new base::ConditionVariable(m_mutex);
}

WorkerThread::WorkerThread() : base::SimpleThread("default"), m_stopping(false), m_started(false), m_complete(false) {
	m_data = new WorkerThreadData();
}

WorkerThread::WorkerThread(const std::string& name) : base::SimpleThread(name),  m_stopping(false), m_started(false), m_complete(false) {
	m_data = new WorkerThreadData();
    m_data->m_name = name;
}

WorkerThread::~WorkerThread()
{
	join();
	delete m_data;
}

void WorkerThread::enqueue(WorkItemPtr workItem)
{
	base::AutoLock lock(*m_data->m_mutex);
	if(!m_stopping)
	{
		m_data->m_workQueue.push(workItem);
		m_data->m_workAdded->Signal();
	}
}

void WorkerThread::join()
{
	// First block further additions to the queue.
	base::AutoLock lock(*m_data->m_mutex);
	m_stopping = true;
	m_data->m_workAdded->Signal();

	// Wait for the thread to finish processing work items.
	while(!m_complete)
	{
		m_data->m_workerDone->Wait();
	}

	// Now join with it.
	this->join();
}

void WorkerThread::run()
{
	m_started = true;
	while(true)
	{
		base::Lock lock;
		if(!m_data->m_workQueue.empty())
		{
			WorkItemPtr workItem = m_data->m_workQueue.front();
			m_data->m_workQueue.pop();
			lock.Release();

			workItem->run();
		}
		else
		{
			if(m_stopping)
				break;

			// Sleep until there is work or shutdown needing done.
			m_data->m_workAdded->Wait();
		}
	}
	m_complete = true;
	m_data->m_workerDone->Signal();
}
