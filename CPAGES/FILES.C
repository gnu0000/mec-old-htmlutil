
/*
 *
 * files.c
 * Tuesday, 4/22/1997.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GnuType.h>
#include <GnuDir.h>
#include <GnuFile.h>
#include <GnuMisc.h>
#include "files.h"


CHAR cSORT;      


/*--- at first we will sort by name --*/
static int compare (PFINFO pfo, PFIL pFil)
   {
   CHAR szExt1[32], szExt2[32];

   switch (tolower (cSORT))
      {
      case SORT_NONE:
         return 1;

      case SORT_EXT :
         *szExt1 = *szExt2 = '\0';
         DirSplitPath (szExt1, pfo->szName, DIR_EXT);
         DirSplitPath (szExt2, pFil->pszOld, DIR_EXT);
         return stricmp (szExt1, szExt2);
         
      case SORT_NAME:
         return stricmp (pfo->szName, pFil->pszOld);

      case SORT_SIZE:
         if (pfo->ulSize > pFil->ulSize)
            return 1;
         return -1;

      case SORT_DATE:
         return 1; // not implemented yet

      default:
         return 1;
      }
   return 0;
   }


static PFIL AddFile (PFIL pFil, PFINFO pfo)
   {
   if (!pFil)
      {
      CHAR szDesc [128];

      pFil          = calloc (1, sizeof (FIL));
      pFil->pszOld  = strdup (pfo->szName);
      pFil->fDate   = pfo->fDate;
      pFil->ulSize  = pfo->ulSize;

      if (FilGet4DosDesc (pfo->szName, szDesc))
         pFil->pszDesc = strdup (szDesc);
      return pFil;
      }
   if (compare (pfo, pFil) < 0)
      pFil->left = AddFile (pFil->left, pfo);
   else
      pFil->right = AddFile (pFil->right, pfo);
   return pFil;
   }


PFIL AddFiles (PSZ pszSpec, PFIL pFil)
   {
   PFINFO pfo, pfoTmp;

   pfo = DirFindAll (pszSpec, FILE_NORMAL);
   for (pfoTmp = pfo; pfoTmp; pfoTmp = pfoTmp->next)
      pFil = AddFile (pFil, pfoTmp);
   DirFindAllCleanup (pfo);
   return pFil;
   }


/*--- no assumptions about order ---*/
static BOOL FindName (PFIL pFil, PSZ pszName)
   {
   if (!pFil)
      return FALSE;
   if (!stricmp (pszName, pFil->pszNew))
      return TRUE;
   if (FindName (pFil->left, pszName))
      return TRUE;
   return FindName (pFil->right, pszName);
   }

void Lower (PSZ psz)
   {
   for (; *psz; psz++)
      *psz = tolower (*psz);
   }


static void MakeName (PFIL pFilTree, PFIL pFil, PSZ pszDir)
   {
   CHAR szName [128], szExt  [128],szNew  [128];
   BOOL bOK = FALSE;
   UINT uDirLen, uExtLen;

   if (pFil->pszNew)
      return; // already has a name

   DirSplitPath (szName, pFil->pszOld, DIR_NAME);
   DirSplitPath (szExt,  pFil->pszOld, DIR_EXT);
   Lower (szName);
   Lower (szExt);
   Lower (pszDir);
   uDirLen = strlen (pszDir);
   uExtLen = strlen (szExt);

   if (!stricmp (szExt, "exe") || !stricmp (szExt, "lib") ||
       !stricmp (szExt, "obj") || !stricmp (szExt, "zip") ||
       !stricmp (szExt, "ebl"))
      {
      strcpy (szNew, pFil->pszOld);
      bOK = TRUE;
      }
   if (!bOK && (strlen (szName) + uExtLen <= 7))
      {
      sprintf (szNew, "%s%s-%s.htm", pszDir, szName, szExt);
      bOK = !FindName (pFilTree, szNew);
      }
   if (!bOK)
      {
      strcpy (szNew, pszDir);
      strcat (szNew, szName);
      szNew[8+uDirLen-uExtLen] = '\0';
      strcat (szNew, szExt);
      strcat (szNew, ".htm");
      bOK = !FindName (pFilTree, szNew);
      }
   if (!bOK)
      {
      strcpy (szNew, szName);
      szNew[7+uDirLen-uExtLen] = '\0';
      strcat (szNew, "-");
      strcat (szNew, szExt);
      strcat (szNew, ".htm");
      bOK = !FindName (pFilTree, szNew);
      }
   if (!bOK)
      Error ("Could not make unique name for %s", pFil->pszOld);

   pFil->pszNew = strdup (szNew);
   }



void MakeNames (PFIL pFilTree, PFIL pFil, PSZ pszDir)
   {
   if (!pFil)
      return;

   MakeName (pFilTree, pFil, pszDir);
   MakeNames (pFilTree, pFil->left, pszDir);
   MakeNames (pFilTree, pFil->right, pszDir);
   }

