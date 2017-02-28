#ifndef _SCHEDEVENTLIST_H
#define _SCHEDEVENTLIST_H

#include <list>

class VizSchedEvent;

class VizSchedEventList {
public:
	VizSchedEventList();

	bool IsEmpty();

	click_t FirstClick();

	// We assume we're locked when this is called
	VizSchedEvent* PopFirst();

	// We assume we're locked when this is called
	void DeleteAll();

	// We assume we're locked when this is called
	void DeleteAllId(int cursorid);

	// We assume we're locked when this is called
	void DeleteAllBefore(int cursorid, click_t beforeclk);

	// We assume we're locked when this is called
	void CheckSanity(const char* tag = "unset");

	// We assume we're locked when this is called
	void InsertEvent(VizSchedEvent* e);

	int NumEvents();
	int NumEventsOfSid(int sid);
	
	std::string DebugString();

private:
	// The list is time-ordered by the click value
	VizSchedEvent* m_first;
	VizSchedEvent* m_last;
};

#endif
