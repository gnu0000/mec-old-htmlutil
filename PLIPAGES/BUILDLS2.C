/*
 *
 * buildls2.c
 * Thursday, 8/29/1996.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <GnuType.h>
#include <GnuFile.h>
#include <GnuStr.h>
#include <GnuMisc.h>
#include <GnuArg.h>
#include <GnuDir.h>
#include <GnuMem.h>



typedef struct _stack
   {
   PSZ pszScope;
   struct _stack *next;
   } STACK;
typedef STACK *PSTACK;

BOOL bDEBUG;

PSTACK pSTK = NULL;

/***************************************************************************/

void _WrtScope (FILE *fp, PSTACK pstk)
   {
   if (!pstk)
      return;
   _WrtScope (fp, pstk->next);
   fprintf (fp, "%s", pstk->pszScope);
   fprintf (fp, ".");
   }

void PushScope (PSZ pszScope)
   {
   PSTACK pstk;

   if (bDEBUG)
      printf ("Pushing %s\n", pszScope);

   pstk = calloc (1, sizeof (STACK));
   pstk->next = pSTK;
   pstk->pszScope = strdup (pszScope);
   pSTK = pstk;

   }

void PopScope (PSZ pszScope)
   {
   PSTACK pstk;

   if (bDEBUG)
      printf ("Popping %s\n", pszScope);

   if (!pSTK)
      Error ("No scope");
   if (pszScope && *pszScope && stricmp (pszScope, pSTK->pszScope))
      Error ("Invalid scoping %s : %s", pszScope, pSTK->pszScope);

   pstk = pSTK->next;
   MemFreeData (pSTK->pszScope);
   MemFreeData (pSTK);
   pSTK = pstk;
   }


//void GetScope (PSZ psz)
//   {
//   PSTK pstk;
//
//   *psz = '\0';
//   for (pstk = pSTK; pstk; pstk = pstk->next)
//      {
//      strcat (psz, pstk->pszScope);
//      strcat (psz, ".");
//      }
//   }



void WriteScope (FILE *fp)
   {
   _WrtScope (fp, pSTK);
   }


/***************************************************************************/

void StrLower (PSZ psz)
   {
   for (; psz && *psz; psz++)
      *psz = tolower (*psz);
   }


void WriteLine (FILE *fp, PSZ pszID, PSZ pszFile, ULONG ulLine)
   {
   fprintf (fp, "%-40s,%12s,%5lu,", pszID, pszFile, ulLine);
   WriteScope (fp);
   fprintf (fp, "\n");
   }


void MakeNames (PSZ pszOut, PSZ pszIn, PSZ pszPath, PSZ pszName)
   {
   PSZ  psz;
   BOOL bHFile;

   StrLower (pszPath);
   StrLower (pszName);
   sprintf  (pszIn,  "%s%s", pszPath, pszName);
   sprintf  (pszOut, "%s%s", pszPath, pszName);

   bHFile = (pszName[strlen (pszName)-1] == 'h');
   if (psz = strrchr (pszOut, '.'))
      *psz = '\0';

   if (bHFile)
      {
      pszOut[strlen (pszPath)+7] = '\0';
      strcat (pszOut, "h");
      }
   strcat (pszOut, ".htm");
   }


void GetID (PSZ pszID, PSZ pszSrc)
   {
   while (__iscsym(*pszSrc))
      *pszID++ = *pszSrc++;
   *pszID++ = '\0';
   }

void ProcessFile (FILE *fpIn, FILE *fpOut, PSZ pszInFile, PSZ pszOutFile)
   {
   CHAR  szLine[512];
   CHAR  szLine2[512];
   CHAR  szID[512];
   ULONG ulLine, ulFilePos;
   PSZ   psz1, psz2, psz3;

   FilSetLine (0);
   while (FilReadLine (fpIn, szLine, "", sizeof (szLine)) != (UINT)-1)
      {
      /*--- look for fn start ---*/
      if (psz1 = strchr (szLine, ':'))
         {
         psz2 = StrSkipBy (psz1+1, " \t");

         if (strnicmp (psz2, "PROC", 4))
            {
            ulFilePos = ftell (fpIn);
            FilReadLine (fpIn, szLine2, "", sizeof (szLine));
            psz3 = StrSkipBy (szLine2, " \t");
            if (strnicmp (psz3, "PROC", 4))
               {
               fseek (fpIn, ulFilePos, SEEK_SET);
               continue;
               }
            }
//         *psz1 = '\0';

         psz2 = StrSkipBy (szLine, " \t");
         ulLine = FilGetLine ();
         GetID (szID, psz2);
         if (!*szID)
            continue;
         psz1 = StrSkipBy (psz2+strlen(szID), " \t");
         if (*psz1 != ':')
            continue;
         WriteLine (fpOut, szID, pszOutFile, ulLine);
         PushScope (szID);
         continue;
         }

      /*--- look for fn end ---*/
      psz1 = StrSkipBy (szLine, " \t");
      if (strnicmp (psz1, "END", 3))
         continue;
      if (psz1[3] != ' ' && psz1[3] != '\t')
         continue;
      psz2 = StrSkipBy (psz1+3, " \t");
      if (!*psz2 || *psz2 == ';')
         continue;
      GetID (szID, psz2);
      PopScope (szID);
      }
   }


void AddReferences (FILE *fp, PSZ pszSpec)
   {
   PFINFO pfo, pfoTmp;
   CHAR   szOutFile[256];
   CHAR   szInFile[256];
   CHAR   szDir[256];

   printf ("Adding refereneces for: %s\n", pszSpec);
   DirSplitPath (szDir,  pszSpec,  DIR_DRIVE | DIR_DIR);
   pfo = DirFindAll (pszSpec, FILE_NORMAL);
   for (pfoTmp = pfo; pfoTmp; pfoTmp = pfoTmp->next)
      {
      MakeNames (szOutFile, szInFile, szDir, pfoTmp->szName);
      WriteLine (fp, szInFile, szOutFile, 1);
      }
   DirFindAllCleanup (pfo);
   }


void Usage (void)
   {
   printf ("BuildLs2  Builds lists of PLI Definition positions %s\n\n", __DATE__);
   printf ("USAGE:  BuildLst [options] files..\n\n");
   printf ("WHERE:  files .... are the PLI files to process.\n");
   printf ("        [options] are 0 or more of:\n");
   printf ("           /Links=file ....... Specify auto link file to build.\n");
   printf ("           /FileRef=files .... Include references to other files\n");
   printf ("\n");
   printf ("Basic Usage:\n");
   printf ("\n");
   printf ("  BUILDLST /Links=PLI.LNK /FileRef=*.pli\n");
   printf ("\n");
   printf ("See PLIPAGES for how to use these files.\n");
   printf ("\n");
   exit (0);
   }


int main(int argc, char *argv[])
   {
   CHAR   szLinksFile[256];
   CHAR   szDir      [256];
   CHAR   szInFile   [256];
   CHAR   szOutFile  [256];
   FILE   *fpOut, *fpIn;
   UINT   i, uFiles;
   PFINFO pfo, pfoTmp;

   if (ArgBuildBlk ("*^Links% *^FileRef=% ? *^C *^H *^Debug"))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk (argv))
      Error ("%s", ArgGetErr ());

   if (ArgIs ("?"))
      Usage ();

   if (!ArgIs (NULL))
      Usage ();

   bDEBUG = ArgIs ("Debug");

   strcpy (szLinksFile, ArgIs ("Links") ? ArgGet ("Links", 0) : "links.lnk");

   if (!(fpOut= fopen (szLinksFile, "wt")))
      Error ("Unable to open links file %s", szLinksFile);

   for (uFiles=i=0; i< ArgIs (NULL); i++)
      {
      DirSplitPath (szDir,  ArgGet (NULL, i), DIR_DRIVE | DIR_DIR);
      pfo = DirFindAll (ArgGet (NULL, i), FILE_NORMAL);
      for (pfoTmp = pfo; pfoTmp; pfoTmp = pfoTmp->next)
         {
         MakeNames (szOutFile, szInFile, szDir, pfoTmp->szName);
         if (!(fpIn = fopen (szInFile, "rt")))
            Error ("Unable to open input file %s", szInFile);
         printf ("processing file %s\n", szInFile);
         ProcessFile (fpIn, fpOut, szInFile, szOutFile);
         fclose (fpIn);
         uFiles++;
         }
      DirFindAllCleanup (pfo);
      }
   for (i=0; i < ArgIs ("FileRef"); i++)
      AddReferences (fpOut, ArgGet ("FileRef", i));
   fclose (fpOut);
   printf ("%d files processed", uFiles);
   return 0;
   }

