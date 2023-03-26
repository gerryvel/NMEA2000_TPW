#ifndef _TASK_H_
#define _TASK_H_


// grundlegene Worte um einen Task Bereich einzugrenzen
#define taskBegin() static int mark = 0; static unsigned long __attribute__((unused)) timeStamp = 0; switch(mark){ case 0:  
#define taskEnd() }


// Task Kontrol Worte, diese werden Taskwechsel einleiten
#define taskSwitch() do { mark = __LINE__; return ; case __LINE__: ; } while (0)
#define taskPause(interval) timeStamp = millis(); while((millis() - timeStamp) < (interval)) taskSwitch()
#define taskWaitFor(condition) while(!(condition)) taskSwitch();

// Benennen und anspringen von Schrittketten Verzweigungen
#define taskStepName(STEPNAME) TASKSTEP_##STEPNAME  :
#define taskJumpTo(STEPNAME)  goto  TASKSTEP_##STEPNAME 

#endif   
