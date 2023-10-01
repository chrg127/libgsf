#pragma once

extern void DisplayError (char *, ...);
extern bool IsTagPresent (unsigned char *);
extern bool IsValidGSF (unsigned char *);
extern void setupSound(void);
extern int GSFRun(char *);
extern void GSFClose(void) ;
extern bool EmulationLoop(void);
