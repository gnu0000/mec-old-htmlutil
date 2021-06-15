/*
 *
 * files.h
 * Tuesday, 4/22/1997.
 *
 */

typedef struct _fil
   {
   PSZ   pszOld;
   PSZ   pszNew;
   PSZ   pszDesc;
   FDATE fDate;
   ULONG ulSize;

   struct _fil *left;
   struct _fil *right;
   }FIL;
typedef FIL *PFIL;


#define SORT_NONE 'x'
#define SORT_EXT  'e'
#define SORT_NAME 'n'
#define SORT_SIZE 's'
#define SORT_DATE 'd'


extern CHAR cSORT;      


void MakeNames (PFIL pFilTree, PFIL pFil, PSZ pszDir);

PFIL AddFiles (PSZ pszSpec, PFIL pFil);

void Lower (PSZ psz);

