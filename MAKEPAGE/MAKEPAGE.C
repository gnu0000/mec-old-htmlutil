/*
 *
 * makehtm.c
 * Tuesday, 8/27/1996.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <GnuType.h>
#include <GnuFile.h>
#include <GnuStr.h>
#include <GnuArg.h>
#include <GnuMem.h>
#include <GnuMisc.h>

typedef struct _lnk
   {
   PSZ  pszSrcName;  // string to make a HREF
   UINT uSrcLen   ;  // len of pszSrcName
   PSZ  pszFile   ;  // file to point to
   UINT uLine     ;  // line to point to
   PSZ  pszGenKey ;  // link key
   PSZ  pszHREF   ;  // HREF string in pre-build form
   UINT uHREFLen  ;  // len of pszHREF
//   BOOL bNoCase   ;

   struct _lnk *leftT;
   struct _lnk *rightT;

   struct _lnk *leftN;
   struct _lnk *rightN;
   } LNK;
typedef LNK *PLNK;



BOOL bCASE;
CHAR szBUFF  [0x4000];
CHAR szBUFF2 [0x4000];


PSZ LoadBuffer (PSZ pszFile)
   {
   FILE *fp;
   UINT uRead;

   if (!(fp = fopen (pszFile, "rt")))
      Error ("Cannot open input file %s", pszFile);
   uRead = fread (szBUFF, 1, sizeof(szBUFF)-1, fp);
   fclose (fp);
   szBUFF[uRead] = '\0';
   return strdup (szBUFF);
   }


PSZ BaseName (PSZ psz, PSZ pszFile)
   {
   PSZ p;

   p = strrchr (pszFile, '\\');
   p = (p ? p+1 : pszFile);
   strcpy (psz, p);
   if (p = strrchr (psz, '.'))
      *p = '\0';
   return psz;
   }

PSZ MyDate (PSZ psz)
   {
   time_t tim;

   tim  = time (NULL);
   strcpy (psz, ctime (&tim));
   psz[10] = '\0';
   return psz;
   }

PSZ MyTime (PSZ psz)
   {
   time_t tim;

   tim  = time (NULL);
   strcpy (psz, ctime (&tim) + 11);
   psz[8] = '\0';
   return psz;
   }

INT Mystrcmp (PSZ psz1, PSZ psz2)
   {
   if (bCASE)
      return strcmp (psz1, psz2);
   stricmp (psz1, psz2);
   }
INT Mystrncmp (PSZ psz1, PSZ psz2, UINT uLen)
   {
   if (bCASE)
      return strncmp (psz1, psz2, uLen);
   strnicmp (psz1, psz2, uLen);
   }


void WriteBlock (FILE *fp, PSZ pszBuff, PSZ pszInFile, PSZ pszOutFile)
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
         if (!Mystrcmp (szWord, "infile"))
            fprintf (fp, "%s", pszInFile);
         else if (!Mystrcmp (szWord, "outfile"))
            fprintf (fp, "%s", pszOutFile);
         else if (!Mystrcmp (szWord, "infilebase"))
            fprintf (fp, "%s", BaseName (sz, pszInFile));
         else if (!Mystrcmp (szWord, "outfilebase"))
            fprintf (fp, "%s", BaseName (sz, pszOutFile));
         else if (!Mystrcmp (szWord, "date"))
            fprintf (fp, "%s", MyDate (sz));
         else if (!Mystrcmp (szWord, "time"))
            fprintf (fp, "%s", MyTime (sz));
         else
            fprintf (fp, "@%s", szWord);
         }
      }
   }


/*************************************************************************/
PLNK plnkTTOP = NULL;
PLNK plnkNTOP = NULL;
UINT uLastCount = 0;


UINT AddToTTree (PLNK plnkTop, PLNK plnkNew)
   {
   PLNK *pplnk;
   INT  i;

   if (!(i = Mystrcmp (plnkNew->pszSrcName, plnkTop->pszSrcName)))
      return printf ("Duplicate link tag: %s\n", plnkNew->pszSrcName);
   pplnk = (i < 0 ? &(plnkTop->leftT) : &(plnkTop->rightT));
   if (*pplnk)
      return AddToTTree (*pplnk, plnkNew);
   *pplnk = plnkNew;
   return 0;
   }


UINT AddToNTree (PLNK plnkTop, PLNK plnkNew)
   {
   PLNK *pplnk;
   INT  i;

   if (!(i = (INT)plnkNew->uLine - (INT)plnkTop->uLine))
      i = Mystrcmp (plnkNew->pszFile, plnkTop->pszFile);
   pplnk = (i < 0 ? &(plnkTop->leftN) : &(plnkTop->rightN));
   if (*pplnk)
      return AddToNTree (*pplnk, plnkNew);
   *pplnk = plnkNew;
   return 0;
   }


PLNK FindTNode (PLNK plnkTop, PSZ pszSrc)
   {
   INT  i;

   if (!plnkTop)
      return NULL;
   if (!(i = Mystrncmp (pszSrc, plnkTop->pszSrcName, plnkTop->uSrcLen)))
      return plnkTop;
   if (i < 0)
      return FindTNode (plnkTop->leftT, pszSrc);
   return FindTNode (plnkTop->rightT, pszSrc);
   }


PLNK FindNNode (PLNK plnkTop, PSZ pszFile, UINT uLine)
   {
   INT  i;

   if (!plnkTop)
      return NULL;
   if (!(i = (INT)uLine - (INT)plnkTop->uLine))
      if (!(i = Mystrcmp (pszFile, plnkTop->pszFile)))
         return plnkTop;
   if (i < 0)
      return FindNNode (plnkTop->leftN, pszFile, uLine);
   return FindNNode (plnkTop->rightN, pszFile, uLine);
   }


/*************************************************************************/
PSZ MakeHREF (PSZ pszBuff, PLNK plnk)
   {
   CHAR szBuff [256];

   sprintf (szBuff, "<A HREF=\"%s#%s\">%s</A>", 
            plnk->pszFile, plnk->pszGenKey, plnk->pszSrcName);

   return strdup (szBuff);
   }


void StoreLink (PPSZ ppsz, UINT uCols)
   {
   PLNK plnk;
   CHAR szBuff [256];

   if (!(plnk = calloc (1, sizeof (LNK))))
      Error ("cant malloc");
   plnk->pszSrcName = ppsz[0];
   plnk->uSrcLen    = strlen(ppsz[0]);
   plnk->pszFile    = ppsz[1];
   plnk->uLine      = atoi(ppsz[2]);
//   plnk->bNoCase    = (uCols >= 4 && ppsz[3] && *ppsz[3]);

   sprintf (szBuff, "ALNK_%s", ppsz[0]);
   plnk->pszGenKey  = strdup (szBuff);
   plnk->pszHREF    = MakeHREF (szBuff, plnk);
   plnk->uHREFLen   = strlen(plnk->pszHREF);

   if (!plnkTTOP)
      plnkTTOP = plnk;
   else
      AddToTTree (plnkTTOP, plnk);

   if (!plnkNTOP)
      plnkNTOP = plnk;
   else
      AddToNTree (plnkNTOP, plnk);
   }


void DumpLinks (PLNK plnk)
   {
   if (!plnk)
      return;

   DumpLinks (plnk->leftT);
   printf ("%s,%s,%d,%s\n", 
            plnk->pszSrcName, plnk->pszFile, plnk->uLine, plnk->pszHREF);
   DumpLinks (plnk->rightT);
   }



void ReadAutolinkFile (PSZ pszFile)
   {
   FILE *fp;
   PPSZ ppsz;
   UINT uCols;

   if (!(fp = fopen (pszFile, "rt")))
      Error ("Cannot open autolink file %s", pszFile);

   while (FilReadLine (fp, szBUFF, "", sizeof (szBUFF)) != (UINT)-1)
      {
      ppsz = StrMakePPSZ  (szBUFF, ",\n", TRUE, TRUE, &uCols);
      if (uCols < 3 || !ppsz[0] || !ppsz[1] || !ppsz[2])
         {
         MemFreePPSZ (ppsz, uCols);
         continue;
         }
      StoreLink (ppsz, uCols);
      free (ppsz); 
      }
   }



void WriteWithLinks (FILE *fpOut, PSZ pszSrc, PSZ pszInFile, PSZ pszOutFile)
   {
   PLNK plnk, plnkHold = NULL;
   UINT cPrev, c;
   BOOL bWrote;

   /*--- see if there is any NAME records to write ---*/

   if (plnkHold = FindNNode (plnkNTOP, pszOutFile, (UINT)FilGetLine ()-1))
      fprintf (fpOut, "<A NAME=\"%s\"></A>\n", plnkHold->pszGenKey);

   /*--- and HREF records to write ---*/
   cPrev = 0;
   while (*pszSrc)
      {
      bWrote = FALSE;
      if (!isalnum (cPrev) && cPrev != '_')
         {
         if (plnk = FindTNode (plnkTTOP, pszSrc))
            {
            c = pszSrc[plnk->uSrcLen];
            if (!isalnum (c) && c != '_'  && (plnk != plnkHold))
               {
               fprintf (fpOut, plnk->pszHREF);
               pszSrc  += plnk->uSrcLen;
               cPrev = 0;
               bWrote = TRUE;
               }
            }
         }
      if (!bWrote)
         fputc (cPrev = *pszSrc++, fpOut);
      }
   }


void WriteBody (FILE *fpOut, FILE *fpIn, PSZ pszInFile, PSZ pszOutFile)
   {
   FilSetLine (0);
   while (FilReadLine (fpIn, szBUFF, "", sizeof (szBUFF)) != (UINT)-1)
      {
      if (plnkTTOP)
         WriteWithLinks (fpOut, szBUFF, pszInFile, pszOutFile);
      else
         fprintf (fpOut, "%s", szBUFF);
      fprintf (fpOut, "\n");
      }
   }


void Usage (void)
   {
   printf ("MakePage  Simple WEB page creation utility                %s\n\n", __DATE__);

   printf ("USAGE:  MakePage [options] infile [outfile]\n\n");
   printf ("WHERE:  infile .... is the name of the flat text to make a page with.\n");
   printf ("        outfile ... If specified, is the name of the html page.\n");
   printf ("                     If not given, the input file + '.htm' is used.\n\n");
   printf ("        [options] are 0 or more of:\n");
   printf ("           /? ................ This help.\n");
   printf ("           /Pre=file ......... Use this HTML prefix file.\n");
   printf ("           /Post=file ........ Use this HTML suffix file.\n");
   printf ("           /Var=string ....... supply value for @# param.\n");
   printf ("           /Links=file ....... Specify auto link file.\n");
   printf ("\n");
   printf ("the prefile and post file can have the following replacable parameters:\n");
   printf ("      @infile ....... The input file name\n");
   printf ("      @outfile ...... The output file name\n");
   printf ("      @infilebase ... The input file name without the extension\n");
   printf ("      @outfilebase .. The output file name without the extension\n");
   printf ("      @date ......... todays date\n");
   printf ("      @time ......... current time\n");
   printf ("      @0 thru @9 .... The nth /Var parameter specified on cmd line.\n");
   printf ("\n");
   printf ("Autolink (CSV) file format is:\n");
   printf ("stringtolink, DestFile, DestLine\n");
   printf ("\n");
   printf ("for example I use:\n");
   printf ("for %%x in (*.c) MAKEPAGE /Post=ebspost.htm %%x\n");
   printf ("This makes the set of EBS source html pages\n");
   printf ("\n");
   exit (0);
   }


int main (int argc, char *argv[])
   {
   FILE *fpIn, *fpOut;
   char szInFile    [256]; 
   char szOutFile   [256]; 
   CHAR szPreFile   [256];
   CHAR szPostFile  [256];
   CHAR szPath      [256];
   PSZ  psz;
   PSZ  pszPre; 
   PSZ  pszPost; 
   UINT i;

   if (ArgBuildBlk ("*^Pre% *^Post% *^Var% *^Links% *^Dump *^CaseSensitive ?"))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk (argv))
      Error ("%s", ArgGetErr ());

   if (ArgIs ("?") || !ArgIs (NULL))
      Usage ();

   bCASE = ArgIs ("CaseSensitive");

   strcpy (szPath, argv[0]);
   if (psz = strrchr (szPath, '\\'))
      psz[1] = '\0';

   if (ArgIs ("Pre"))
      strcpy (szPreFile, ArgGet ("Pre", 0));
   else
      strcat (strcpy (szPreFile, szPath), "PRE.HTM");

   if (ArgIs ("Post"))
      strcpy (szPostFile, ArgGet ("Post", 0));
   else
      strcat (strcpy (szPostFile, szPath), "POST.HTM");

   pszPre  = LoadBuffer (szPreFile);
   pszPost = LoadBuffer (szPostFile);

   for (i=0; i < ArgIs ("Links"); i++)
      ReadAutolinkFile (ArgGet ("Links", i));

   if (ArgIs ("Dump"))
      {
      DumpLinks (plnkTTOP);
      exit (0);
      }

   strcpy (szInFile, ArgGet(NULL, 0));
   if (ArgIs (NULL) > 1)
      strcpy (szOutFile, ArgGet(NULL, 1));
   else
      {
      strcpy (szOutFile, szInFile);
      if (psz = strrchr (szOutFile, '.'))
         *psz = '\0';
      }
   if (!strchr (szOutFile, '.'))
      strcat (szOutFile, ".HTM");

   if (!(fpIn = fopen (szInFile, "rt")))
      Error ("Cannot open input file %s", szInFile);

   if (!(fpOut = fopen (szOutFile, "wt")))
      Error ("Cannot open output file %s", szOutFile);

   printf ("Processing File %s to %s ...", szInFile, szOutFile);
   WriteBlock (fpOut, pszPre, szInFile, szOutFile);
   WriteBody (fpOut, fpIn, szInFile, szOutFile);
   WriteBlock (fpOut, pszPost, szInFile, szOutFile);
   fclose (fpIn);
   fclose (fpOut);
   printf ("Done\n");
   return 0;
   }

