#ifndef	__usignal_h
#define	__usignal_h

typedef	void	Sigfunc(int);	/* for signal handlers */

void sig_chld(int signo);
Sigfunc* signal(int signo, Sigfunc *func);
Sigfunc* Signal(int signo, Sigfunc *func);

#endif	/* __usignal_h */