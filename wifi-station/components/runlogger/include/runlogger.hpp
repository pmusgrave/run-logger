#include "run.hpp"

// This is the class that handles main UI
class RunLogger {
public:
	RunLogger();
	~RunLogger();
	void start_run(void);
	void pause_run(void);
	void stop_run(void);
	void reset(void);

private:
	Run current_run;
};