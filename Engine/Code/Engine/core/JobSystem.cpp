#include "Engine/core/JobSystem.hpp"


JobWorkerThread::JobWorkerThread(int id, JobSystem* jobSystem)
	: m_Id(id)
	, m_jobSystem(jobSystem)
{	
	m_thread = new std::thread(&JobWorkerThread::ThreadMain, this);
}

JobWorkerThread::~JobWorkerThread()
{
	m_thread->join(); 
}

void JobWorkerThread::ThreadMain()
{
	// Spins continuously, looking for unclaimed Job work to claim 
	// repeating this until signaled to stop looping and exit
	while (!m_jobSystem->m_isShuttingDown)
	{
		Job* jobToExecute = m_jobSystem->ClaimJob();
		if (jobToExecute)
		{
			jobToExecute->Execute();
			m_jobSystem->ReceiveCompletedJob(jobToExecute);
		}
		else
		{
			// there is also std::chrono::hours, minutes, seconds, milliseconds, microseconds, nanoseconds to use
			std::this_thread::sleep_for((std::chrono::microseconds(1))); // sleeping for 1ms if none job is found
		}
	}
}

JobSystem::JobSystem(JobSystemConfig const& config)
	: m_config(config)
{

}

JobSystem::~JobSystem()
{

}

void JobSystem::Startup()
{
	int numCores = std::thread::hardware_concurrency(); // get the numbers of threads that could run at the same time
	if (m_config.numberOfWorkers == -1)
	{
		CreateWorkers(numCores - 1); // because the main game is already taking one thread
	}
	else
	{
		CreateWorkers(m_config.numberOfWorkers);
	}
}

void JobSystem::ShutDown()
{
	m_isShuttingDown = true;
}

void JobSystem::CreateWorkers(int numWorkers)
{
	for (int workerIndex = 0; workerIndex < numWorkers; ++workerIndex)
	{
		JobWorkerThread* worker = new JobWorkerThread(workerIndex, this);
		m_workers.push_back(worker);
	}
}

void JobSystem::DestroyAllWorkers()
{
	for (int i = 0; i < (int)m_workers.size(); ++i)
	{
		delete m_workers[i];
		m_workers[i] = nullptr;
	}
}

Job* JobSystem::ClaimJob()
{ 
	m_queuedJobsMutex.lock();
	if (!m_queuedJobs.empty())
	{
		Job* claimedJob = m_queuedJobs.front();
		m_queuedJobs.pop_front();
		m_queuedJobsMutex.unlock();
		claimedJob->m_jobStatus = JobStatus::CLAIMED; // todo: job status should be changed after mutex unlock?
		return claimedJob;
	}
	else
	{
		m_queuedJobsMutex.unlock();
		Job* claimedJob = nullptr;
		return claimedJob;
	}
}

void JobSystem::ReceiveCompletedJob(Job* job)
{
	m_completedJobsMutex.lock();
	m_completedJobs.push_back(job);
	m_completedJobsMutex.unlock();
	job->m_jobStatus = JobStatus::COMPLETED;
}

Job* JobSystem::RetrieveCompletedJobs(Job* requestedJob)
{
	if (requestedJob) // request a specific job
	{
		m_completedJobsMutex.lock();
		std::deque<Job*>::iterator iter;
		for (iter = m_completedJobs.begin(); iter != m_completedJobs.end(); ++iter)
		{
			if (*iter == requestedJob)
			{
				Job* retrievedjob = *iter;
				retrievedjob->m_jobStatus = JobStatus::RETRIEVED;
				m_completedJobsMutex.unlock();
				return retrievedjob;
			}
		}
		m_completedJobsMutex.unlock(); 
		return nullptr; // did not find the specific job
	}
	else
	{
		// not requested specific job
		m_completedJobsMutex.lock();
		if (!m_completedJobs.empty()) // return the front one when the deque is not empty
		{
			Job* retrievedjob = m_completedJobs.front();
			m_completedJobs.pop_front();
			retrievedjob->m_jobStatus = JobStatus::RETRIEVED;
			m_completedJobsMutex.unlock();
			return retrievedjob;
		}
		else
		{
			// return null when the completed job deque is empty
			m_completedJobsMutex.unlock();
			return nullptr;
		}
	}
}

int JobSystem::GetNumQueuedJobs() const
{
	m_queuedJobsMutex.lock();
	int numJobs = (int)m_queuedJobs.size();
	m_queuedJobsMutex.unlock();
	return numJobs;
}

void JobSystem::QueueJobs(Job* jobToQueue)
{
	m_queuedJobsMutex.lock();
	m_queuedJobs.push_back(jobToQueue);
	m_queuedJobsMutex.unlock();
}

Job::~Job()
{

}
