/*
 *
 * buffer.h
 * Tuesday, 4/22/1997.
 *
 */

extern PSZ pszPRE;
extern PSZ pszBODY;
extern PSZ pszPOST;

void WriteBlock (FILE *fp, PSZ pszBuff, PFIL pFil);

void LoadCfg (PSZ pszDefault, PSZ pszParm, PSZ pszArg0);

