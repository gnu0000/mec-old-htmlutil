/*
 *
 * buffer.c
 * Tuesday, 4/22/1997.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <GnuType.h>
#include <GnuMem.h>
#include <GnuArg.h>
#include <GnuDir.h>
#include <GnuMisc.h>
#include <GnuStr.h>
#include <GnuCfg.h>
#include <GnuFile.h>
#include "files.h"
#include "buffers.h"


PSZ pszPRE;
PSZ pszBODY;
PSZ pszPOST;


static PSZ MyDate (PSZ psz)
   {
   time_t tim;

   tim  = time (NULL);
   strcpy (psz, ctime (&tim));
   psz[10] = '\0';
   return psz;
   }

static PSZ MyTime (PSZ psz)
   {
   time_t tim;

   tim  = time (NULL);
   strcpy (psz, ctime (&tim) + 11);
   psz[8] = '\0';
   return psz;
   }


void WriteBlock (FILE *fp, PSZ pszBuff, PFIL pFil)
   {
   CHAR sz[128], szWord[256];
   PSZ  psz;
   UINT c;

   for (psz = pszBuff; c = *psz; )
      {
      if (c != '@')
         {
         fputc (*psz++, fp);
         continue;
         }
      psz++;
      if (*psz >= '0' && *psz <= '9')
         fprintf (fp, "%s", ArgGet ("Var", *psz++ - '0'));
      else if (*psz == '@')
         fprintf (fp, "@", psz++);
      else
         {
         StrGetIdent (&psz, szWord, FALSE, FALSE);

         if (!stricmp (szWord, "date"))
            fprintf (fp, "%s", MyDate (sz));
         else if (!stricmp (szWord, "time"))
            fprintf (fp, "%s", MyTime (sz));

         else if (!stricmp (szWord, "oldfile"))
            fprintf (fp, "%s", (pFil ? pFil->pszOld : ""));
         else if (!stricmp (szWord, "infile"))
            fprintf (fp, "%s", (pFil ? pFil->pszOld : ""));
         else if (!stricmp (szWord, "infilebase"))
            fprintf (fp, "%s", (pFil ? DirSplitPath (sz, pFil->pszOld, DIR_NAME) : ""));

         else if (!stricmp (szWord, "newfile"))
            fprintf (fp, "%s", (pFil ? pFil->pszNew : ""));
         else if (!stricmp (szWord, "outfile"))
            fprintf (fp, "%s", (pFil ? pFil->pszNew : ""));
         else if (!stricmp (szWord, "outfilebase"))
            fprintf (fp, "%s", (pFil ? DirSplitPath (sz, pFil->pszNew, DIR_NAME) : ""));

         else if (!stricmp (szWord, "filedate"))
            fprintf (fp, "%s", (pFil ? StrFDateString (szWord, pFil->fDate) : ""));
         else if (!stricmp (szWord, "filesize"))
            fprintf (fp, "%6lu", (pFil ? pFil->ulSize : 0));
         else if (!stricmp (szWord, "filedesc"))
            fprintf (fp, "%s", pFil && pFil->pszDesc ? pFil->pszDesc : "");
         else
            fprintf (fp, "@%s", szWord);
         }
      }
   }


/*
 * loads pszPRE pszBODY and pszPOST from the cfg file
 *
 */
void LoadCfg (PSZ pszDefault, PSZ pszParm, PSZ pszArg0)
   {
   CHAR szName[256];
   PSZ  psz;
   UINT uRet;

   if (pszParm)
      strcpy (szName, pszParm);
   else if (!access (pszDefault, 0))
      strcpy (szName, pszDefault);
   else if (pszArg0 && strchr (pszArg0, '\\'))
      {
      if (psz = strrchr (strcpy (szName, pszArg0), '\\'))
         psz[1] = '\0';
      strcat (szName, pszDefault);
      }
   else
      Error ("Can't find cfg file %s", pszDefault);

   if (uRet = CfgGetSection (szName, "Header", &pszPRE,  0))
      {
      if (uRet == 1)
         Error ("Cannot locate config file %s", szName);
      Error ("Cannot load [Header] section from file %s", szName);
      }

   CfgGetSection (szName, "Body",   &pszBODY, 0);

   if (CfgGetSection (szName, "Footer", &pszPOST, 0))
      Error ("Cannot load [Footer] section from file %s", szName);
   }

