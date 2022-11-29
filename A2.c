#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#define MASTER_TO_SLAVE_TAG 1
#define SLAVE_TO_MASTER_TAG 4

char * GetFiles(int *max, int*length) //Function that get Names of all files in folder
{
    int n=0, i=0,s=0,j=0,k=0;
    DIR *d;
    struct dirent *dir;
    d = opendir("Aristo-Mini-Corpus"); //Folder Name
//Determine the number of files
    while((dir = readdir(d)) != NULL)
    {
        if ( !strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") )
        {
        }
        else
        {
            n++;
        }
    }
    rewinddir(d);
    char *filesMatrix[n];
    int sizes[n];
//Put file names into the array
    while((dir = readdir(d)) != NULL)
    {
        if ( !strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") )
        {}
        else
        {
            s=strlen(dir->d_name);
            filesMatrix[i] = (char*) malloc (s+1);
            strncpy (filesMatrix[i],dir->d_name, strlen(dir->d_name) );
            sizes[i]=s;
            i++;
            if(s>*max)
                *max=s;
        }
    }
    int count=0;
    *length=*max*n;
    char* fileList=malloc(*length);
    for (i=0; i<n; i++)
    {
        if(sizes[i]<*max)
        {
            for(k=0; k<(*max-sizes[i]); k++)
            {
                fileList[count++]='*';
            }
        }
        for(j=0; j<sizes[i]; j++)
        {
            fileList[count++]=filesMatrix[i][j];
        }
    }
    rewinddir(d);
    return fileList;
}
char *read_line(FILE *fin)
{
    char *buffer;
    char *tmp;
    int read_chars = 0;
    int bufsize = 512;
    char *line = malloc(bufsize);
    buffer = line;

    while ( fgets(buffer, 1000 - read_chars, fin) )
    {
        read_chars = strlen(line);

        if ( line[read_chars - 1] == '\n' )
        {
            line[read_chars - 1] = '\0';
            return line;
        }

        else
        {
            bufsize = 2 * bufsize;
            tmp = realloc(line, bufsize);
            if ( tmp )
            {
                line = tmp;
                buffer = line + read_chars;
            }
            else
            {
                free(line);
                return NULL;
            }
        }
    }
    return NULL;
}
char* Search(char* FName,char* Word,int *num)
{
    FILE *fin;
    char *line;
    char *result="";
    char * tmp;
    char* Directory="Aristo-Mini-Corpus\\";
    char* path=malloc(strlen(Directory)+strlen(FName)+2);
    char* NEWLINNE="\n";
    strcpy(path,Directory);
    strcat(path,FName);
    fin = fopen(path, "r");
//    printf("\n@Test1 Path:%s\n",path);
    if ( fin )
    {
        while ( line = read_line(fin) )
        {
            if ( strstr(line, Word) )
            {
                *num+=1;
                tmp = malloc(3 + strlen(result)+ strlen(line) );
                strcpy(tmp, result);
                strcat(tmp, NEWLINNE);
                strcat(tmp, line);
                result=tmp;
            }
            free(line);
        }
    }
    return result;
}
int main(int argc, char * argv[])
{
    int i=0,j=0,k=0,max=0,x=0;
    int rank;
    int size; //num of procceses
    int num=0; //Number of results
    int portion=0,remain=0,Length=0;
    char* files;
    char*localPart;
    char*localResult="";
    char* word="kinetic energy";
    char* FinalResult;
    MPI_Status status;
    MPI_Request request;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
//===========================================================
    if (rank == 0)
    {
        files=GetFiles(&max,&Length);
        int numOfFiles= Length/max;
        portion=(numOfFiles/size)*max;
        remain=(numOfFiles%size)*max;
        /* printf("Enter Word..\n");
         scanf("%s", &word);*/
        x=strlen(word)+1;
    }
    MPI_Bcast(&portion, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&max, 1, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Bcast(&x, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&word, x, MPI_CHAR, 0, MPI_COMM_WORLD);

    localPart = malloc(sizeof(char)*portion);
    MPI_Scatter(files, portion, MPI_CHAR,localPart, portion, MPI_CHAR, 0, MPI_COMM_WORLD);
//    return;
    if (rank !=0)
    {
        //printf("\nThis IS Rank: %d \nThis is LocalPart:#%s# \n",rank,localPart);
        char* fileName=malloc(sizeof(char)*max);
        for(i=0; i<portion; i+=max)
        {
            for(j=0; j<max; j++)
            {
                fileName[j]=localPart[i+j];
            }
            for(j=0; j<max; j++)
            {
                if (fileName[0]=='*')
                    fileName++;
                else
                    break;
            }
            char*tmp1=Search(fileName,word,&num);
            char*tmp2=malloc(strlen(tmp1)+strlen(localResult));
            strcpy(tmp2, localResult);
            strcat(tmp2, tmp1);
            localResult=tmp2;
        }
        int done;
        MPI_Isend(&num, 1, MPI_INT, 0, SLAVE_TO_MASTER_TAG, MPI_COMM_WORLD, &request);
        MPI_Recv(&done, 1, MPI_INT, 0, MASTER_TO_SLAVE_TAG, MPI_COMM_WORLD, &status);
        FILE *f=fopen("Output.txt","a");
        fprintf(f,"%s",localResult);
        fclose(f);
        printf("\nRank:%d \nFinal Num:%d\n",rank,num);
    }
    if (rank==0)
    {
        char* NewLine="\n";
        printf("@test5");
        int FinalNum=0,FinalSize=0,localSize=0;
        char* RemainNames=malloc(sizeof(char)*remain);
        for(i=portion*size; i<strlen(files); i+=max)
        {
            for(j=0; j<max; j++)
            {
                RemainNames[j]=files[i+j];
            }
        }
        char* Masterpart=malloc(strlen(RemainNames)+strlen(localPart));
        strcpy(Masterpart,localPart);
        strcat(Masterpart,RemainNames);
        char* fileName=malloc(sizeof(char)*max);
        for(i=0; i<strlen(Masterpart); i+=max)
        {
            for(j=0; j<max; j++)
            {
                fileName[j]=Masterpart[i+j];
            }
            for(j=0; j<max; j++)
            {
                if (fileName[0]=='*')
                    fileName++;
                else
                    break;
            }
            char*tmp1=Search(fileName,word,&num);
            char*tmp2=malloc(strlen(tmp1)+strlen(localResult));
            strcpy(tmp2, localResult);
            strcat(tmp2, tmp1);
            localResult=tmp2;
        }
        FinalNum=num;
        printf("test8");
        for(i=1; i<size; i++)
        {
            MPI_Recv(&num, 1, MPI_INT, i, SLAVE_TO_MASTER_TAG, MPI_COMM_WORLD, &status);
            FinalNum+=num;
            printf("test(2)");
        }
        FILE *f=fopen("Output.txt","w");
        fprintf(f,"Word: %s\nSearch Results Found: %d",word,FinalNum);
        fclose(f);
        int done=1;
        for(i=1; i<size; i++)
        {
            MPI_Isend(&done, 1, MPI_INT, i, MASTER_TO_SLAVE_TAG, MPI_COMM_WORLD, &request);
        }
        f=fopen("Output.txt","a");
        printf("\nBnTest\nResult: %s",localResult);
        fprintf(f,"%s",localResult);
        fclose(f);
    }
    MPI_Finalize();
    return 0;
}
