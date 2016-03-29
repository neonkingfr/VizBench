#ifndef _SCHEDEVENTLIST_H
#define _SCHEDEVENTLIST_H

#include <list>

class SchedEvent;

class SchedEventList {
public:
	SchedEventList();

	bool IsEmpty();

	click_t FirstClick();

	// We assume we're locked when this is called
	SchedEvent* PopFirst();

	// We assume we're locked when this is called
	void DeleteAll();

	// We assume we're locked when this is called
	void DeleteAllId(int cursorid);

	// We assume we're locked when this is called
	void DeleteAllBefore(int cursorid, click_t beforeclk);

	// We assume we're locked when this is called
	void CheckSanity(const char* tag = "unset");

	// We assume we're locked when this is called
	void InsertEvent(SchedEvent* e);

	int NumEvents();
	int NumEventsOfSid(int sid);
	
	std::string DebugString();

private:
	// The list is time-ordered by the click value
	SchedEvent* m_first;
	SchedEvent* m_last;
};

#endif
