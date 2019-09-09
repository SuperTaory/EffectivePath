#include <iostream>
#include <cstdlib>
#include "tinyxml.h"
#include <cstring>
#include <cstdio>
#include <set>
#include <algorithm>
#include "tinystr.h"
#include "tinyxmlerror.cpp"
#include "tinyxmlparser.cpp"
#include "tinyxml.cpp"
#include "tinystr.cpp"

using namespace std;
#define Timemax 200                    //设定路径的最长时间为200分钟
#define ValidPathStationMax 150        //设定每个OD点有效路径包含的站点数
#define AllPathFile             ".\\ResultFiles\\allpath.txt"            //存储所有OD点间的所有有效路径  work
#define ShortPathFile           ".\\ResultFiles\\shortpath.txt"          //存储所有OD点间的最短路径  work
//#define GetShortTime            ".\\ResultFiles\\ShortTime.txt"          //存储最短路径的时间 格式：起点编号-终点编号-最短时间  work
//FILE* ShortTimeFormat;
FILE *AllPathfp;    /*储存所有路径的文件*/
FILE *ShortPathfp;  /*储存最短路径的文件*/
char **CopyStationName;                                     //车站名称的复制 用于处理调查数据

//XML定义换乘站点所属的运营商
typedef struct XMLStationConect{
    char StatNo[11];
    bool IsTrans;
    char LineNo[4];
    char PreStatNo[11];
    double PreStatDis;
    double StatStopTime1;
    int PreDirect;
    char NextStatNo[11];
    double NextStatDis;
    double StatStopTime2;
    int NextDirect;
}XMLStationConnectInfo;

//XML--转换（转换车站编号为1-n、线路编号为1-n）
typedef struct XMLConvertStationConect{
    int StatNo;
    bool IsTrans;
    int LineNo;
    int PreStatNo;
    double PreStatDis;
    double StatStopTime1;
    int PreDirect;
    int NextStatNo;
    double NextStatDis;
    double StatStopTime2;
    int NextDirect;
}XMLConvertStationConnectInfo;

//XML换乘站步行时间
typedef struct XMLTransferStationWakltime
{
    char StationNo[11];
    char FromLineNo[4];
    char ToLineNo[4];
    double TranswalkUpToUp;
    double TranswalkUpToDown;
    double TranswalkDownToUp;
    double TranswalkDownToDown;
}XMLTransferStatWakltime;

//XMLOD点票价
typedef struct XMLODStationPrice
{
    char Ori[11];
    char Des[11];
    int Price;
}XMLO_DStatPrice;

//定义站点的邻接矩阵
typedef struct MetroStation{
    int vertex;
    struct MetroStation *NextStation;
}*Metrograph;

//站间距
typedef struct StationDis{
    int StationNum;
    int PreviStation;
    double PreviDis;
    int PreDirect;
    int NextStation;
    double NextDis;
    int NextDirect;
}StaDis;

//站间运行时间列表
typedef struct StationTime{
    int StationNum;
    int PreviStation;
    double PreviTime;
    int PreDirect;
    int NextStation;
    double NextTime;
    int NextDirect;
}StaTime;

//储存有效路径查找点
typedef struct SearNode{
    int Prev;
    int NodeNum;
}SearStation;

//换乘站步行时间
typedef struct TransStationWalkTime{
    int Stationnum;
    int Fromline;
    int Toline;
    double TranswalkTime[2][2]; //分别表示上行-上行、上行-下行、下行-上行、下行-下行
}TransStaWalkTime;

//换乘站的换乘时间
typedef struct Trans{
    int Stationnum;
    int Fromline;
    int Toline;
    double TransTime[2][2]; //分别表示上行-上行、上行-下行、下行-上行、下行-下行
}Transtime;

//车站停站时间(按照上下行考虑，起点―目的点时目的点的停站时间)
typedef struct StationStopTime{
    int PreStationNo;
    int TOStationNo;
    double ToStaStopTime;
}ODStatStopTime;

int Level = -1;     /*指示查找路径队列的位置*/
int NotValidCount = 0; /*统计无效路径条数*/
int ValidPathCount = 0;
int ChangeTimeCount = 0; /* 统计因换乘站修改最短路径的次数*/
int Transcount = 0;    //储存路径中换乘的次数
double TransSumtime = 0;  //储存路径的换乘时间
int TransLimitCount = 0; //限制有效路径的换乘次数
double ValidPathTime = 0; //用于记录有效路径的出行时间限制
double *tempRecordsOfTime = nullptr;
double tempSumTime = 0;


//字符串拼接
char* PreStrcat(const char* AtrriteName,char *buffer, int index, int bufferContent)
{
    memset(buffer,'\0',bufferContent);
    char *Tmpbuffer = new char[bufferContent];
    strcpy(buffer, AtrriteName);
    itoa(index,Tmpbuffer,10);
    strcat(buffer, Tmpbuffer);
    delete[] Tmpbuffer;
    return buffer;
}

//站点到站点的时间(即站间运行时间+站内停车时间，若涉及换乘站则加上换乘时间)
double StatoSta(int Pre, int Sou, int Des, const bool *IsTrans, int AllStationCount, int AdjaceMaxCount, StaTime *StatTime, ODStatStopTime *Statstoptime,
                int **StationLine, int TransStation, Transtime *TransTime)
{
    double Alltime = 0;
    int FromLine = 0;
    int ToLine = 0;
    int index = 0;
    int preDirect = 0;
    int nextDirect = 0;
    double tempStopTime = 0;
    //确定换乘时的predirect和nextDirect
    if(IsTrans[Sou-1])
    {
        for(index = 0; index < AllStationCount; index++)
        {
            if(StatTime[index].StationNum == Sou)
            {
                if(StatTime[index].PreviStation == Des)
                {
                    Alltime = StatTime[index].PreviTime;
                    nextDirect = StatTime[index].PreDirect;
                    break;
                }

                else if(StatTime[index].NextStation == Des)
                {
                    Alltime = StatTime[index].NextTime;
                    nextDirect = StatTime[index].NextDirect;
                    break;
                }
            }
        }

        for(index = 0; index < AllStationCount; index++)
        {
            if(StatTime[index].StationNum == Pre)
            {
                if(StatTime[index].PreviStation == Sou)
                {
                    preDirect = StatTime[index].PreDirect;
                    break;
                }

                else if(StatTime[index].NextStation == Sou)
                {
                    preDirect = StatTime[index].NextDirect;
                    break;
                }
            }
        }
    }
    else
    {
        for(int indexnum = 0; indexnum < AdjaceMaxCount; indexnum++)
        {
            if(Statstoptime[indexnum].PreStationNo == Sou&&
               Statstoptime[indexnum].TOStationNo == Des)
            {
                tempStopTime = Statstoptime[indexnum].ToStaStopTime;
            }
        }
        for(index = 0; index < AllStationCount; index++)
        {
            if(StatTime[index].StationNum == Sou)
            {
                if(Pre == 0)
                {
                    if(StatTime[index].PreviStation == Des)
                        return StatTime[index].PreviTime;
                    if(StatTime[index].NextStation == Des)
                        return StatTime[index].NextTime;
                }
                else
                {
                    if(StatTime[index].PreviStation == Des)
                    {
                        return StatTime[index].PreviTime + tempStopTime;
                    }
                    else if(StatTime[index].NextStation == Des)
                    {
                        return StatTime[index].NextTime + tempStopTime;
                    }
                }
            }
        }
    }
    if(Pre == 0)
        return Alltime;

//    set<int> setPre(StationLine[Pre-1], StationLine[Pre-1]+sizeof(StationLine[0][0]));
//    set<int> setSou(StationLine[Sou-1], StationLine[Sou-1]+sizeof(StationLine[0][0]));
//    set<int> setDes(StationLine[Des-1], StationLine[Des-1]+sizeof(StationLine[0][0]));
    set<int> setPre, setSou, setDes, setFromLine, setToLine;
    for (int i = 0; i < 4 && StationLine[Pre-1][i] != 0; i++)
        setPre.insert(StationLine[Pre-1][i]);
    for (int i = 0; i < 4 && StationLine[Sou-1][i] != 0; i++)
        setSou.insert(StationLine[Sou-1][i]);
    for (int i = 0; i < 4 && StationLine[Des-1][i] != 0; i++)
        setDes.insert(StationLine[Des-1][i]);

    set_intersection(setPre.begin(), setPre.end(), setSou.begin(),setSou.end(), inserter(setFromLine, setFromLine.begin()));
    set_intersection(setSou.begin(), setSou.end(), setDes.begin(),setDes.end(), inserter(setToLine, setToLine.begin()));

    if (setFromLine.size() == 1 && setToLine.size() == 1){
        FromLine = *setFromLine.begin();
        ToLine = *setToLine.begin();
    }
    else if(setFromLine.size() > 1 && setToLine.size() == 1){
        FromLine = 8;  //8表示11号线,因为地铁线路图中红树湾南-车公庙相邻且同属9和11号线路，但实际乘车时两站若相邻则同属于11号线
        ToLine = *setToLine.begin();
    }
    else if(setFromLine.size() == 1 && setToLine.size() > 1){
        FromLine = *setFromLine.begin();
        ToLine = 8;
    }
    else{
        cout<<"Wrong happend!"<<endl;
        exit(-1);
    }


//    //查看节点前后节点的所属线路，查看是否换乘
//    if(StationLine[Pre-1][0]!=0 && StationLine[Sou-1][0]!=0 && StationLine[Pre-1][0] == StationLine[Sou-1][0])
//        FromLine = StationLine[Pre-1][0];
//    else if(StationLine[Pre-1][0]!=0 && StationLine[Sou-1][1]!=0 && StationLine[Pre-1][0] == StationLine[Sou-1][1])
//        FromLine = StationLine[Pre-1][0];
//    else if(StationLine[Pre-1][1]!=0 && StationLine[Sou-1][0]!=0 && StationLine[Pre-1][1] == StationLine[Sou-1][0])
//        FromLine = StationLine[Pre-1][1];
//    else if(StationLine[Pre-1][1]!=0 && StationLine[Sou-1][1]!=0 && StationLine[Pre-1][1] == StationLine[Sou-1][1])
//        FromLine = StationLine[Pre-1][1];
//
//    if(StationLine[Sou-1][0]!=0 && StationLine[Des-1][0]!=0 && StationLine[Sou-1][0] == StationLine[Des-1][0])
//        ToLine = StationLine[Sou-1][0];
//    else if(StationLine[Sou-1][0]!=0 && StationLine[Des-1][1]!=0 && StationLine[Sou-1][0] == StationLine[Des-1][1])
//        ToLine = StationLine[Sou-1][0];
//    else if(StationLine[Sou-1][1]!=0 && StationLine[Des-1][0]!=0 && StationLine[Sou-1][1] == StationLine[Des-1][0])
//        ToLine = StationLine[Sou-1][1];
//    else if(StationLine[Sou-1][1]!=0 && StationLine[Des-1][1]!=0 && StationLine[Sou-1][1] == StationLine[Des-1][1])
//        ToLine = StationLine[Sou-1][1];

    //没有换乘线路
    if(FromLine == ToLine)
    {
        for(int indexnum = 0; indexnum < AdjaceMaxCount; indexnum++)
        {
            if(Statstoptime[indexnum].PreStationNo == Sou &&
               Statstoptime[indexnum].TOStationNo == Des)
            {
                return Alltime+Statstoptime[indexnum].ToStaStopTime;
            }
        }
    }
    //换乘了线路
    else
    {
        Transcount++;
        for(index = 0; index < 2*TransStation; index++)
        {
            if(TransTime[index].Stationnum == Sou && TransTime[index].Fromline == FromLine&&
               TransTime[index].Toline == ToLine)
            {
                Alltime += TransTime[index].TransTime[preDirect][nextDirect];
                TransSumtime += TransTime[index].TransTime[preDirect][nextDirect];
                return Alltime;
            }
        }
    }
    return Alltime;
}

//计算相邻站点之间的距离，若不相邻则返回Timemax
double Graphtime(int Pre, int Sou, int Des, struct MetroStation *Head, bool *IsTrans, int AllStationCount, int AdjaceMaxCount, StaTime *StatTime,
                 ODStatStopTime *Statstoptime,int **StationLine, int TransStation, Transtime *TransTime)
{
    Metrograph ptr;
    ptr = Head[Sou-1].NextStation;
    while(ptr != nullptr)
    {
        if(ptr->vertex == Des)
            return StatoSta(Pre, Sou, Des, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                            StationLine, TransStation, TransTime);
        else
            ptr = ptr->NextStation;
    }
    return Timemax;
}

void StorShortTime(int begin, const double *Pstor,int indexnum, double **StaShortTime)  //储存最短时间信息到txt文件
{
    char buffer[20];
    memset(buffer,'\0',20);
    char next = '\n';
    char span=' ';
    for(int i = 1; i <= indexnum; i++)
    {

        StaShortTime[begin-1][i-1] = *(Pstor+i);
        sprintf(buffer,"%d",begin);
        fwrite(buffer,strlen(buffer),1,ShortPathfp);
//        fwrite(CopyStationName[begin-1],strlen(CopyStationName[begin-1]),1,ShortTimeFormat);

        memset(buffer,0x00,sizeof(buffer));
        sprintf(buffer,"%c",span);
        fwrite(buffer,strlen(buffer),1,ShortPathfp);
//        fwrite(buffer,strlen(buffer),1,ShortTimeFormat);

        sprintf(buffer,"%d",i);
        fwrite(buffer,strlen(buffer),1,ShortPathfp);
//        fwrite(CopyStationName[i-1],strlen(CopyStationName[i-1]),1,ShortTimeFormat);

        memset(buffer,0x00,sizeof(buffer));
        sprintf(buffer,"%c",span);
        fwrite(buffer,strlen(buffer),1,ShortPathfp);
//        fwrite(buffer,strlen(buffer),1,ShortTimeFormat);

        sprintf(buffer,"%.4f",*(Pstor+i));
        fwrite(buffer,strlen(buffer),1,ShortPathfp);
//        fwrite(buffer,strlen(buffer),1,ShortTimeFormat);

        sprintf(buffer,"%c",next);
        fwrite(buffer,strlen(buffer),1,ShortPathfp);
//        fwrite(buffer,strlen(buffer),1,ShortTimeFormat);

    }
}

//由于换乘时间影响最短时间，需要对最短时间进行修正,如大剧院、老街附近站点
double ChangeShortTime(int vertex,SearStation *queue, const int *shortvisited,const double *costtime,struct MetroStation *Head, bool *IsTrans,
                       int AllStationCount, int AdjaceMaxCount, StaTime *StatTime, ODStatStopTime *Statstoptime,
                       int **StationLine, int TransStation, Transtime *TransTime)
{
    int pre = 0;
    int sou = 0;
    int des = 0;
    int preed = 0;
    int soued = 0;
    Metrograph ptr;
    double shorttime = costtime[vertex];
    double newtime;
    soued = queue[vertex-1].Prev; //源站点Begin
    preed = queue[soued-1].Prev;  //0
    ptr = Head[soued-1].NextStation; //遍历源站点对应的拉链
    while(ptr != nullptr)
    {
        sou = ptr->vertex;  //当前遍历站点的编号
        if(sou != preed && shortvisited[sou] == 1 && queue[sou-1].Prev != soued)
        {
            pre = queue[sou-1].Prev;
            newtime = costtime[sou]+Graphtime(pre, sou, soued, Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                                              StationLine, TransStation, TransTime)+Graphtime(sou, soued, vertex, Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                                                                                              StationLine, TransStation, TransTime);
            if(newtime < shorttime) {
                shorttime = newtime;
                ChangeTimeCount++;
            }
        }
        ptr = ptr->NextStation;
    }
    return shorttime;
}

//查找最短路径,即begin节点到其他节点的最短路径
void Dijkstra(int Begin, int StationMaxCount, struct MetroStation *Head, bool *IsTrans, int AllStationCount, int AdjaceMaxCount,
              StaTime *StatTime, ODStatStopTime *Statstoptime,int **StationLine, int TransStation, Transtime *TransTime,
              double** StaShortTime)
{
    double MinTime;   //最小数值用于标号
    int Vertex;       //用于记录标号的节点信息
    int index;
    int Edges;
    int *ShortVisited = new int[StationMaxCount+1]();      //标记已经得出最短出行时间的站点号
    auto *CostTime = new double[StationMaxCount+1];    //记录路径的出行时间
    auto *ShortQueue = new SearStation[StationMaxCount]; //储存查找的节点信息

    for(index = 0; index <= StationMaxCount; index++)
        CostTime[index] = Timemax;

    for(index = 0; index < StationMaxCount; index++)
    {
        ShortQueue[index].Prev = 0;
        ShortQueue[index].NodeNum = 0;
    }

    Edges = 1;
    ShortVisited[Begin] = 1;

    for(index =1; index <= StationMaxCount; index++)
    {
        CostTime[index] = Graphtime(0, Begin, index, Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                                    StationLine, TransStation, TransTime);
        ShortQueue[index-1].NodeNum = index;
        ShortQueue[index-1].Prev = Begin;
    }


    CostTime[Begin] = 0;
    Vertex = Begin;
    ShortQueue[Begin-1].NodeNum = Begin;
    ShortQueue[Begin-1].Prev = 0;

    while(Edges < StationMaxCount)
    {
        Edges++;
        MinTime = Timemax;
        //从未确定最短路径的站点集中找出其中路径最短的点
        for(index =1; index <= StationMaxCount; index++)
        {
            if(ShortVisited[index] == 0 && MinTime > CostTime[index])
            {
                Vertex = index;
                MinTime = CostTime[index];
            }
        }
        //若源站是换乘站则重新计算最短时间
        if(IsTrans[ShortQueue[Vertex-1].Prev-1])
        {
            CostTime[Vertex] = ChangeShortTime(Vertex,ShortQueue,ShortVisited,CostTime
                    ,Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime, StationLine, TransStation, TransTime);
        }
        ShortVisited[Vertex] = 1;

        for( index = 1;index <= StationMaxCount; index++)
        {
            //根据刚才确定最短路径的站点更新剩余未确定最短路径的站点到源站点的最短路径距离
            if(ShortVisited[index] == 0)
            {
                double temp = Graphtime(ShortQueue[Vertex-1].Prev, Vertex, index,
                                        Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                                        StationLine, TransStation, TransTime);
                if(CostTime[Vertex] + temp < CostTime[index]) //判断是否存在更优的最短路径
                {
                    CostTime[index] = CostTime[Vertex] + temp;
                    ShortQueue[index-1].Prev = Vertex;
                }
            }
        }
    }
    //存储最短路径信息
    StorShortTime(Begin,CostTime,StationMaxCount, StaShortTime);

    delete []ShortVisited;      //标记已经得出最短出行时间的站点号
    delete []CostTime;          //记录路径的出行时间
    delete []ShortQueue;        //储存查找的节点信息

}
/*******************************/
/*有效路径的时间限制*/
/*******************************/
double ShortAddLimit = 0;        //短距离有效路径能够增加的时间
double MiddleAddLimit = 0;        //中距离有效路径能够增加的时间
double LongAddLimit = 0;          //长距离有效路径能够增加的时间
double ShortTravelLimit = 0;        //短距离时间建议被限制在30分钟以内
double MiddleTravelLimit = 0;        //中距离时间建议被限制在30～60分钟以内
/*******************************/
/*查找所有可能的路径*/
/*******************************/

/*向查找队列存入节点信息*/
int PushQueue(SearStation Node, int StationMax, SearStation *SearchQueue, int *Visited)
{
    if(Level >= StationMax-1)
        return -1;
    else
    {
        Level++;
        SearchQueue[Level].NodeNum = Node.NodeNum;
        SearchQueue[Level].Prev = Node.Prev;
        Visited[Node.NodeNum] = 1;
    }
    return 0;
}

/*取出查找队列的节点信息*/
int PullQueue(int *Visited, SearStation* SearchQueue)
{
    if(Level < 0)
        return -1;

    Visited[SearchQueue[Level].NodeNum] = 0;
    SearchQueue[Level].NodeNum = 0;
    SearchQueue[Level].Prev = 0;

    Level--;
    return 0;
}

/*建立轨道交通路网的邻接图形*/
void Creategraph(int** node, int num, struct MetroStation* Head)
{
    Metrograph newnode;                /* 新顶点指标*/
    Metrograph ptr;
    int from;                          /* 边线的起点*/
    int to;                            /* 边线的终点*/
    int i;

    //将node_to加入到from的拉链后面
    for ( i = 0; i < num; i++ )        /* 读取边线的回路*/
    {
        from = node[i][0];              /* 边线的起点*/
        to = node[i][1];                /* 边线的终点*/
        /* 建立新顶点记忆体 */
        newnode = (Metrograph) malloc(sizeof(struct MetroStation));
        newnode->vertex = to;           /* 建立顶点内容*/
        newnode->NextStation = nullptr;    /* 设定指标初值*/
        ptr = &(Head[from-1]);          /* 顶点位置*/
        while ( ptr->NextStation != nullptr )   /* 遍历至链表尾*/
            ptr = ptr->NextStation;           /* 下一个顶点*/
        ptr->NextStation = newnode;          /* 插入结尾*/
    }
    //打印图的信息
    for (int x =0; x < 166; x++){
        cout<<Head[x].vertex<<':';
        Metrograph p = Head[x].NextStation;
        while(p!= nullptr){
            cout<<p->vertex<<'-';
            p = p->NextStation;
        }
        cout<<endl;
    }

}

/*删除建立的邻接轨道交通路网,从头节点紧邻节点开始释放*/
void DeleteGraph(int num, struct MetroStation* Head)
{
    Metrograph ptr;
    Metrograph next;
    for(int index = 0; index < num; index++)
    {
        ptr = Head[index].NextStation;
        while(ptr != nullptr)
        {
            next = ptr;
            ptr = ptr->NextStation;
            free(next);
        }
        Head[index].NextStation = nullptr;
    }
}

//查看是否所有的路径都查找完
bool Check(int BackSource, Metrograph point, SearStation* SearchQueue)
{
    return !(SearchQueue[Level].NodeNum == BackSource && point == nullptr);
}

//向下查找节点邻接的节点
Metrograph FindNextNode(Metrograph point, struct MetroStation* Head, SearStation* SearchQueue)
{
    int NodeNum = 0;
    Metrograph ptr;
    NodeNum = SearchQueue[Level].NodeNum;
    ptr =  Head[NodeNum-1].NextStation;
    while(ptr != nullptr && ptr->vertex != point->vertex)
    {
        ptr = ptr->NextStation;
    }
    if(ptr == nullptr)
    {
        return nullptr;
    }
    else
    {
        ptr = ptr->NextStation;
    }
    return ptr;
}


void PrintNode(SearStation* SearchQueue)
{
    char Stor[5];
    for(int i = 0 ; i <= Level; i++)
    {
        memset(Stor,'\0',5);
        sprintf(Stor,"%d ",SearchQueue[i].NodeNum);
        fwrite(Stor,strlen(Stor),1,AllPathfp);
    }
}

double SumPathTime(int desstation, SearStation* SearchQueue,struct MetroStation *Head, bool *IsTrans, int AllStationCount, int AdjaceMaxCount,
                   StaTime *StatTime, ODStatStopTime *Statstoptime,int **StationLine, int TransStation, Transtime *TransTime)
{
    double SumTime = 0;
    int index;
    if(Level == 0)
    {
        SumTime = Graphtime(0, SearchQueue[0].NodeNum, desstation, Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                            StationLine, TransStation, TransTime);
    }
    else
    {
        if(Level > 0)
        {
            SumTime = Graphtime(0, SearchQueue[0].NodeNum, SearchQueue[1].NodeNum, Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                                StationLine, TransStation, TransTime);
            for(index = 1; index < Level; index++)
            {
                SumTime += Graphtime(SearchQueue[index-1].NodeNum, SearchQueue[index].NodeNum, SearchQueue[index+1].NodeNum, Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                                     StationLine, TransStation, TransTime);
                //如果超出了有效路径的时间范围则停止计算并返回
                if (SumTime > ValidPathTime)
                    return Timemax;
            }
            SumTime += Graphtime(SearchQueue[Level-1].NodeNum, SearchQueue[Level].NodeNum, desstation, Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                                 StationLine, TransStation, TransTime);
        }
    }
    return SumTime;
}

int SearchPth(int SourceNode, int DestinationNode, int StationMax,  SearStation *SearchQueue, int *Visited, struct MetroStation *Head,
              bool *IsTrans, int AllStationCount, int AdjaceMaxCount, StaTime *StatTime, ODStatStopTime *Statstoptime,int **StationLine,
              int TransStation,Transtime *TransTime, double** StaShortTime)

{
    cout<<"searchpath working..."<<endl;
    int counter = 0;
    int NodeNum = 0;       /*用于得到查找节点的节点号*/
    double PathSumTime = 0; //用于记录每条有效路径的总出行时间
    double temptime = 0;
    char Stor[11];
    memset(Stor,'\0',11);
    char span=' ';
    char Nextline = '\n';
    char Pathflag = '#';
    char ValidTimeFlag = 'V';
    char NotValidTimeFlag = 'N';
    bool ExistPathFlag = false;
    double minTime = 200.0;
    int ValidPthCount[ValidPathStationMax];
    double CopyTranscount = 0.0;          //用于记录不存在满足所有约束条件的有效路径时，还需要给出一条路径时的换乘次数拷贝
    double CopyTransSumtime = 0.0;        //用于记录不存在满足所有约束条件的有效路径时，还需要给出一条路径时的换乘时间拷贝
    double CopyPathSumTime = 0.0;         //用于记录不存在满足所有约束条件的有效路径时，还需要给出一条路径时的总出行时间拷贝
    //当起点站和终点站相同时
    if(SourceNode == DestinationNode)
    {
        memset(Stor,'\0',11);
        sprintf(Stor,"%d",SourceNode);
        fwrite(Stor,strlen(Stor),1,AllPathfp);
        fwrite(&span, 1,1,AllPathfp);

        memset(Stor,'\0',11);
        sprintf(Stor,"%d",DestinationNode);
        fwrite(Stor,strlen(Stor),1,AllPathfp);

        fwrite(&span, 1,1,AllPathfp);
        fwrite(&Pathflag,1,1,AllPathfp);
        memset(Stor,'\0',11);
        Transcount = 0;
        sprintf(Stor,"%d",Transcount);
        fwrite(Stor,strlen(Stor),1,AllPathfp);

        fwrite(&span, 1,1,AllPathfp);
        fwrite(&ValidTimeFlag,1,1,AllPathfp);
        fwrite(&span, 1,1,AllPathfp);
        memset(Stor,'\0',11);
        TransSumtime = 0.0;
        sprintf(Stor,"%.4f",TransSumtime);
        fwrite(Stor,strlen(Stor),1,AllPathfp);

        fwrite(&span, 1,1,AllPathfp);
        memset(Stor,'\0',11);
        PathSumTime = 0.0;
        sprintf(Stor,"%.4f",PathSumTime);
        fwrite(Stor,strlen(Stor),1,AllPathfp);

        fwrite(&Nextline,1,1,AllPathfp);

        ValidPathCount++;
        cout<<"searchpath done of "<<ValidPathCount<<endl;
        return 0;
    }
    else
    {
        SearStation NewNode;
        NewNode.NodeNum = SourceNode;
        NewNode.Prev = 0;
        PushQueue(NewNode, StationMax, SearchQueue, Visited); //把源点压入队列

        //设置有效路劲的时间范围
        if(StaShortTime[SourceNode-1][DestinationNode-1] <= ShortTravelLimit)
            ValidPathTime = StaShortTime[SourceNode-1][DestinationNode-1] + ShortAddLimit;
        else if(StaShortTime[SourceNode-1][DestinationNode-1] <= MiddleTravelLimit)
            ValidPathTime = StaShortTime[SourceNode-1][DestinationNode-1] + MiddleAddLimit;
        else
            ValidPathTime = StaShortTime[SourceNode-1][DestinationNode-1] + LongAddLimit;

        struct MetroStation DeletePoint{}; /*用于保存要删除节点的信息*/

        Metrograph point = Head[SourceNode-1].NextStation; //取源点第一个相邻的节点
        do
        {
            while(point != nullptr)
            {
                if(Visited[point->vertex] == 1 || point->vertex == DestinationNode)
                {
                    if(point->vertex == DestinationNode)
                    {
                        counter++;             //记录找到的可达路径的条数
                        Transcount = 0;       //每条有效路径的换乘次数在计算时先清零
                        TransSumtime = 0;     //每条有效路径的换乘时间在计算时先清零

                        PathSumTime = SumPathTime(DestinationNode, SearchQueue, Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime,
                                                  Statstoptime, StationLine, TransStation, TransTime);

                        if(Transcount <= TransLimitCount&&                             //限制换乘规定次数如2次
                           StaShortTime[SourceNode-1][DestinationNode-1] < Timemax)   //当节点之间是连通时才能为有效的路径
                        {
                            if(PathSumTime <= ValidPathTime)
                            {
                                ExistPathFlag = true;
                                PrintNode(SearchQueue);
                                sprintf(Stor,"%d",DestinationNode);
                                fwrite(Stor,strlen(Stor),1,AllPathfp);

                                fwrite(&span, 1,1,AllPathfp);
                                fwrite(&Pathflag,1,1,AllPathfp);
                                sprintf(Stor,"%d",Transcount);
                                fwrite(Stor,strlen(Stor),1,AllPathfp);

                                fwrite(&span, 1,1,AllPathfp);
                                fwrite(&ValidTimeFlag,1,1,AllPathfp);
                                fwrite(&span, 1,1,AllPathfp);
                                sprintf(Stor,"%.4f",TransSumtime);
                                fwrite(Stor,strlen(Stor),1,AllPathfp);

                                fwrite(&span, 1,1,AllPathfp);
                                sprintf(Stor,"%.4f",PathSumTime);
                                fwrite(Stor,strlen(Stor),1,AllPathfp);

                                fwrite(&Nextline,1,1,AllPathfp);

                            }
                            else
                            {
                                if(minTime > PathSumTime)
                                {
                                    minTime = PathSumTime;
                                    for(int & stationNum : ValidPthCount)
                                    {
                                        stationNum = 0;
                                    }
                                    for(int i = 0 ; i <= Level; i++)
                                    {
                                        ValidPthCount[i] = SearchQueue[i].NodeNum;
                                    }
                                    CopyTranscount = Transcount;
                                    CopyTransSumtime = TransSumtime;
                                    CopyPathSumTime = PathSumTime;
                                }
                            }
                        }
                    }
                    point = FindNextNode(point, Head, SearchQueue);
                }
                else
                {
                    NewNode.NodeNum = point->vertex;
                    NewNode.Prev = SearchQueue[Level].NodeNum;
                    PushQueue(NewNode, StationMax, SearchQueue, Visited);            /*插入当前遍历的point节点*/
                    if(Level==1)
                        temptime = Graphtime(0, SearchQueue[0].NodeNum, SearchQueue[1].NodeNum, Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                                             StationLine, TransStation, TransTime);
                    else
                        temptime = Graphtime(SearchQueue[Level-2].NodeNum, SearchQueue[Level-1].NodeNum, SearchQueue[Level].NodeNum, Head, IsTrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,
                                             StationLine, TransStation, TransTime);
                    tempRecordsOfTime[Level] = temptime;
                    tempSumTime += temptime;
                    if (tempSumTime < ValidPathTime){
                        NodeNum = SearchQueue[Level].NodeNum; //队列首节点即point的站点编号
                        point = Head[NodeNum-1].NextStation; //取point节点的下一个相邻节点
                    }
                    else{
                        point = nullptr;
                    }
                }
                
            }
            if(SearchQueue[Level].NodeNum != SourceNode)
            {
                DeletePoint.vertex = SearchQueue[Level].NodeNum;
                DeletePoint.NextStation = nullptr;
                PullQueue(Visited, SearchQueue);
                tempSumTime -= tempRecordsOfTime[Level+1];
                tempRecordsOfTime[Level+1] = 0;
                point = FindNextNode(&DeletePoint, Head, SearchQueue);                   /*返回查找上一层节点的紧邻节点*/
            }
        }while(Check(SourceNode, point, SearchQueue));  //当point为空并且队列首节点为源节点时退出循环
        Level = -1;
        Visited[SourceNode] = 0;
        tempSumTime = 0;
        memset(tempRecordsOfTime, 0, StationMax);

        if(!ExistPathFlag)      //当没有完全满足约束条件的有效路径时，需要记录最接近的有效路径
        {
            NotValidCount++;
            char Storl[5];
            int i = 0;
            while(ValidPthCount[i] != 0)
            {
                memset(Storl, '\0', 5);
                sprintf(Storl, "%d ", ValidPthCount[i++]);
                fwrite(Storl, strlen(Storl), 1, AllPathfp);
            }

            sprintf(Storl, "%d", DestinationNode);
            fwrite(Storl, strlen(Storl), 1, AllPathfp);

            fwrite(&span, 1,1,AllPathfp);
            fwrite(&Pathflag,1,1,AllPathfp);
            sprintf(Storl, "%d", CopyTranscount);
            fwrite(Storl, strlen(Storl), 1, AllPathfp);

            fwrite(&span, 1,1,AllPathfp);
            fwrite(&NotValidTimeFlag,1,1,AllPathfp);
            fwrite(&span, 1,1,AllPathfp);
            sprintf(Storl, "%.4f", CopyTransSumtime);
            fwrite(Storl, strlen(Storl), 1, AllPathfp);

            fwrite(&span, 1,1,AllPathfp);
            sprintf(Storl, "%.4f", CopyPathSumTime);
            fwrite(Storl, strlen(Storl), 1, AllPathfp);

            fwrite(&Nextline,1,1,AllPathfp);

        }
        ValidPathCount++;
        cout<<"searchpath done of "<<ValidPathCount<<",total:"<<counter<<endl;
        return 1;
    }
}


/*#############################################*/
/*主函数                                   */
/*#############################################*/
/*返回值说明：
 1、导入XML文件出错；
 2、XML文件格式出错；
 3、打开中间存储文本出错；
 4、正确返回；
 */
int main()
{
    /*********************************************************/
    /*对配置文件XML进行读取开始                              */
    /*********************************************************/
    int XMLindex = 0;
    int XMLTrainIntervalIndex = 1;
    int XMLSpeedIndex = 1;
    int XMLLineNoNameIndex = 1;
    char XMLTrainIntervalbuffer[20];
    char XMLSpeedbuffer[20];
    char XMLLineNoNamebuffer[20];
    //车站基本信息参数
    int XMLstaitonCount = 0;
    int XMLtransStation = 0;
    int XMLrepeatAllStaitonCount = 0;
    int XMLlineCount = 0;
    int XMLtransLimitCount = 0;
    double *XMLTrainInterval = nullptr;
    double *XMLSpeed = nullptr;
    char **XMLLineNoName = nullptr;
    int XMLLowestODPrice = 0;

    double XMLShortAddLimit = 0;           //短距离有效路径能够增加的时间
    double XMLMiddleAddLimit = 0;          //中距离有效路径能够增加的时间
    double XMLLongAddLimit = 0;            //长距离有效路径能够增加的时间
    double XMLShortTravelLimit = 0;        //短距离时间被限制在30分钟以内
    double XMLMiddleTravelLimit = 0;       //中距离时间被限制在30～60分钟以内

    XMLStationConnectInfo *XMLStatConnectInformation;
    XMLTransferStatWakltime *XMLTransfStatWalktime;
    XMLO_DStatPrice *XMLODStatPrice;

    int XMLReadInforIndex = 0;
    bool LoadXmlResult;

    //解析xml的信息
    auto* myDocument = new TiXmlDocument();
    LoadXmlResult = myDocument->LoadFile("RailTransitInformation.xml");
    if(!LoadXmlResult)
    {
        cout<<"xml error"<<endl;
        return 1;            //导入XML文件出错
    }

    TiXmlElement* rootElement = myDocument->RootElement();  //MetroInformation
    if(strcmp(rootElement->Value(),"MetroInformation") != 0)
    {
        cout << "XML格式错误！"<<endl;
        return 2;
    }

    TiXmlElement* stationElement = rootElement->FirstChildElement();
    if(strcmp(stationElement->Value(),"BaseInformation") != 0)
    {
        cout << "XML格式错误！"<<endl;
        return 2;
    }


    /*读取基本信息，供设置相关数组大小*/
    TiXmlElement* stationElement1 = stationElement->FirstChildElement();
    while(stationElement1)
    {
        TiXmlAttribute* attributeOfStation = stationElement1->FirstAttribute();
        while(attributeOfStation)
        {
            if(!strcmp(attributeOfStation->Name(),"staitonCount"))
            {
                XMLstaitonCount = atoi(attributeOfStation->Value());
            }
            else if(!strcmp(attributeOfStation->Name(),"transStation"))
            {
                XMLtransStation = atoi(attributeOfStation->Value());
            }
            else if(!strcmp(attributeOfStation->Name(),"repeatAllStaitonCount"))
            {
                XMLrepeatAllStaitonCount = atoi(attributeOfStation->Value());
            }
            else if(!strcmp(attributeOfStation->Name(),"lineCount"))
            {
                XMLlineCount = atoi(attributeOfStation->Value());
                XMLTrainInterval = new double[XMLlineCount];
                XMLSpeed = new double[XMLlineCount];
                XMLLineNoName = new char* [XMLlineCount];
                for(XMLindex = 0; XMLindex < XMLlineCount; XMLindex++)
                {
                    XMLLineNoName[XMLindex] = new char[4];
                }
            }

            else if(!strcmp(attributeOfStation->Name(),"transLimitCount"))
            {
                XMLtransLimitCount = atoi(attributeOfStation->Value());
            }
            else if(!strcmp(attributeOfStation->Name(),PreStrcat("TrainInterval",XMLTrainIntervalbuffer, XMLTrainIntervalIndex, strlen("TrainInterval")+2)))
            {
                XMLTrainInterval[XMLTrainIntervalIndex-1] = atof(attributeOfStation->Value());
                XMLTrainIntervalIndex++;

            }
            else if(!strcmp(attributeOfStation->Name(),PreStrcat("Speed",XMLSpeedbuffer, XMLSpeedIndex,strlen("Speed")+2)))
            {
                XMLSpeed[XMLSpeedIndex-1] = atof(attributeOfStation->Value());
                XMLSpeedIndex++;
            }
            else if(!strcmp(attributeOfStation->Name(),PreStrcat("LineNoName",XMLLineNoNamebuffer, XMLLineNoNameIndex, strlen("LineNoName")+2)))
            {
                strcpy(XMLLineNoName[XMLLineNoNameIndex-1], attributeOfStation->Value());
                XMLLineNoNameIndex++;
            }

            else if(!strcmp(attributeOfStation->Name(),"LowestODStationPrice"))
            {
                XMLLowestODPrice = atof(attributeOfStation->Value());
            }
            else if(!strcmp(attributeOfStation->Name(),"ShortTravelLimit"))
            {
                XMLShortTravelLimit = atof(attributeOfStation->Value());
            }
            else if(!strcmp(attributeOfStation->Name(),"MiddleTravelLimit"))
            {
                XMLMiddleTravelLimit = atof(attributeOfStation->Value());
            }
            else if(!strcmp(attributeOfStation->Name(),"ShortAddLimit"))
            {
                XMLShortAddLimit = atof(attributeOfStation->Value());
            }
            else if(!strcmp(attributeOfStation->Name(),"MiddleAddLimit"))
            {
                XMLMiddleAddLimit = atof(attributeOfStation->Value());
            }
            else if(!strcmp(attributeOfStation->Name(),"LongAddLimit"))
            {
                XMLLongAddLimit = atof(attributeOfStation->Value());
            }
            attributeOfStation = attributeOfStation->Next();
        }
        stationElement1 = stationElement1->NextSiblingElement();
    }
    stationElement = stationElement->NextSiblingElement();

    /************************************/
    /*读取站点相关信息*/
    /************************************/
    //设置读取的站点信息数组
    XMLStatConnectInformation = new XMLStationConnectInfo[XMLrepeatAllStaitonCount];
    XMLTransfStatWalktime = new XMLTransferStatWakltime[XMLtransStation*2+18];
    XMLODStatPrice = new XMLO_DStatPrice[XMLstaitonCount*(XMLstaitonCount-1)];
    while(stationElement)
    {
        XMLReadInforIndex = 0;
        TiXmlElement* stationElement2 = stationElement->FirstChildElement();
        while(stationElement2)
        {
            TiXmlAttribute* attributeOfStation = stationElement2->FirstAttribute();
            while(attributeOfStation)
            {

                //读取邻接站点信息
                if(!strcmp(stationElement->Value(), "StationConnectInfor"))
                {
                    if(!strcmp(attributeOfStation->Name(),"StatNo"))
                    {
                        strcpy(XMLStatConnectInformation[XMLReadInforIndex].StatNo, attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"IsTrans"))
                    {
                        XMLStatConnectInformation[XMLReadInforIndex].IsTrans = 1 == atoi(attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"LineNo"))
                    {
                        strcpy(XMLStatConnectInformation[XMLReadInforIndex].LineNo,attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"PreStatNo"))
                    {
                        strcpy(XMLStatConnectInformation[XMLReadInforIndex].PreStatNo,attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"PreStatDis"))
                    {
                        XMLStatConnectInformation[XMLReadInforIndex].PreStatDis = atof(attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"StatStopTime1"))
                    {
                        XMLStatConnectInformation[XMLReadInforIndex].StatStopTime1 = atof(attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"NextStatNo"))
                    {
                        strcpy(XMLStatConnectInformation[XMLReadInforIndex].NextStatNo,attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"NextStatDis"))
                    {
                        XMLStatConnectInformation[XMLReadInforIndex].NextStatDis = atof(attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"StatStopTime2"))
                    {
                        XMLStatConnectInformation[XMLReadInforIndex].StatStopTime2 = atof(attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"PreDirect"))
                    {
                        XMLStatConnectInformation[XMLReadInforIndex].PreDirect = atoi(attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"NextDirect"))
                    {
                        XMLStatConnectInformation[XMLReadInforIndex].NextDirect = atoi(attributeOfStation->Value());
                        XMLReadInforIndex++;
                    }
                    else
                    {
                        //不做任何处理
                    }
                }
                //读取换乘站步行时间
                else if(!strcmp(stationElement->Value(), "TransStationWalkTime"))
                {
                    if(!strcmp(attributeOfStation->Name(),"StationNo"))
                    {
                        strcpy(XMLTransfStatWalktime[XMLReadInforIndex].StationNo, attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"FromLineNo"))
                    {
                        strcpy(XMLTransfStatWalktime[XMLReadInforIndex].FromLineNo, attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"ToLineNo"))
                    {
                        strcpy(XMLTransfStatWalktime[XMLReadInforIndex].ToLineNo, attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"direction00"))
                    {
                        XMLTransfStatWalktime[XMLReadInforIndex].TranswalkUpToUp = atof(attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"direction01"))
                    {
                        XMLTransfStatWalktime[XMLReadInforIndex].TranswalkUpToDown = atof(attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"direction10"))
                    {
                        XMLTransfStatWalktime[XMLReadInforIndex].TranswalkDownToUp = atof(attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"direction11"))
                    {
                        XMLTransfStatWalktime[XMLReadInforIndex].TranswalkDownToDown = atof(attributeOfStation->Value());
                        XMLReadInforIndex++;
                    }
                    else
                    {
                        //不做任何处理
                    }
                }
                //读取O-D点票价信息
                else if(!strcmp(stationElement->Value(), "ODPrice"))
                {
                    if(!strcmp(attributeOfStation->Name(),"Ori"))
                    {
                        strcpy(XMLODStatPrice[XMLReadInforIndex].Ori,  attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"Des"))
                    {
                        strcpy(XMLODStatPrice[XMLReadInforIndex].Des,attributeOfStation->Value());
                    }
                    else if(!strcmp(attributeOfStation->Name(),"Price"))
                    {
                        XMLODStatPrice[XMLReadInforIndex].Price = atof(attributeOfStation->Value());
                        XMLReadInforIndex++;
                    }
                    else
                    {
                        //不做任何处理
                    }
                }
                attributeOfStation = attributeOfStation->Next();
            }
            stationElement2 = stationElement2->NextSiblingElement();
        }
        stationElement = stationElement->NextSiblingElement();
    }

    /*********************************************************/
    /*对配置文件XML进行读取（结束）                          */
    /*********************************************************/
    int Row = 0;
    int Col = 0;
    int Index = 0;

    /*********************************************************/
    /*XML读取线路配置信息数据转成清分需要的格式（开始）      */
    /*********************************************************/

    int StationMaxCount = XMLstaitonCount;                //站点个数
    int TransStationCount = XMLtransStation;            //换乘站的个数
    int AllStationCount = XMLrepeatAllStaitonCount;        //所有节点个数 =站点个数+换乘站个数
    int LineCount = XMLlineCount;                    //线路的条数
    TransLimitCount = XMLtransLimitCount;       //限定换乘次数、
    int AdjaceMaxCount = XMLrepeatAllStaitonCount*2 - LineCount*2;  //邻接节点关系队列最大容量 = 所有节点个数*2 - 线路条数个数*2


    /*每条线路的列车发车间隔*/
    auto *TrainInterval = new double[LineCount];
    for(Row = 0; Row < LineCount; Row++)
    {
        TrainInterval[Row] = XMLTrainInterval[Row];
    }

    /*每条线路的列车速度的影响        */
    auto *Speed = new double[LineCount];
    for(Row = 0; Row < LineCount; Row++)
    {
        Speed[Row] = XMLSpeed[Row];
    }

    /*线路名称*/
    char **LineNoName = new char* [LineCount];
    for(Row = 0; Row < LineCount; Row++)
    {
        LineNoName[Row] = new char[4];
    }
    for(Row = 0; Row < LineCount; Row++)
    {
        strcpy(LineNoName[Row], XMLLineNoName[Row]);
    }

    ShortAddLimit = XMLShortAddLimit;           //短距离有效路径能够增加的时间
    MiddleAddLimit = XMLMiddleAddLimit;          //中距离有效路径能够增加的时间
    LongAddLimit = XMLLongAddLimit;            //长距离有效路径能够增加的时间
    ShortTravelLimit = XMLShortTravelLimit;        //短距离时间被限制在30分钟以内
    MiddleTravelLimit = XMLMiddleTravelLimit;       //中距离时间被限制在30～60分钟以内


    /*定义站点编号数组用于转换*/
    char **ConvertStationNo = new char* [StationMaxCount];
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        ConvertStationNo[Row] = new char[11];
        memset(ConvertStationNo[Row],'\0',11);
    }

    //去除掉重复的站点
    for(Row = 0, Col = 0; Row < StationMaxCount && Col < XMLrepeatAllStaitonCount;)
    {
        bool NewStation = false;
        for(Index = 0; Index < Row; Index++)
        {
            if(!strcmp(XMLStatConnectInformation[Col].StatNo, ConvertStationNo[Index]))
            {
                NewStation = true;
                break;
            }
        }

        if(NewStation)
        {
            Col++;
        }
        else
        {
            strcpy(ConvertStationNo[Row],XMLStatConnectInformation[Col].StatNo);
            Row++;
            Col++;
        }
    }

    //转存站点编号
    char **StationName = new char* [StationMaxCount];
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        StationName[Row] = new char[11];
    }
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        strcpy(StationName[Row], ConvertStationNo[Row]);
    }

    //另外拷贝一份
    CopyStationName = new char* [StationMaxCount];
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        CopyStationName[Row] = new char[11];
    }
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        strcpy(CopyStationName[Row], ConvertStationNo[Row]);
    }

    //把XML数据根据车站编号、线路编号进行转换
    auto *XMLConvertStatConnectInfor = new XMLConvertStationConnectInfo[AllStationCount];
    for(Row = 0; Row < AllStationCount; Row++)
    {
        XMLConvertStatConnectInfor[Row].IsTrans = XMLStatConnectInformation[Row].IsTrans;
        XMLConvertStatConnectInfor[Row].PreStatDis = XMLStatConnectInformation[Row].PreStatDis;
        XMLConvertStatConnectInfor[Row].StatStopTime1 = XMLStatConnectInformation[Row].StatStopTime1;
        XMLConvertStatConnectInfor[Row].NextStatDis = XMLStatConnectInformation[Row].NextStatDis;
        XMLConvertStatConnectInfor[Row].StatStopTime2 = XMLStatConnectInformation[Row].StatStopTime2;
        XMLConvertStatConnectInfor[Row].PreDirect = XMLStatConnectInformation[Row].PreDirect;
        XMLConvertStatConnectInfor[Row].NextDirect = XMLStatConnectInformation[Row].NextDirect;
        for(Col = 0; Col < StationMaxCount; Col++)
        {
            if(!strcmp(XMLStatConnectInformation[Row].StatNo, ConvertStationNo[Col]))
            {
                XMLConvertStatConnectInfor[Row].StatNo = Col+1; //将10位表示的站点编号转化为1-n表示的站点编号
                break;
            }
        }
        for(Col = 0; Col < LineCount; Col++)
        {
            if(!strcmp(XMLStatConnectInformation[Row].LineNo, XMLLineNoName[Col]))
            {
                XMLConvertStatConnectInfor[Row].LineNo = Col+1; //将3位表示的线路编号转化为1-n表示的线路编号
                break;
            }
        }

        if(!strcmp(XMLStatConnectInformation[Row].PreStatNo, "0"))
        {
            XMLConvertStatConnectInfor[Row].PreStatNo = 0;
        }
        else
        {
            for(Col = 0; Col < StationMaxCount; Col++)
            {
                if(!strcmp(XMLStatConnectInformation[Row].PreStatNo, ConvertStationNo[Col]))
                {
                    XMLConvertStatConnectInfor[Row].PreStatNo = Col+1;
                    break;
                }
            }
        }


        if(!strcmp(XMLStatConnectInformation[Row].NextStatNo, "0"))
        {
            XMLConvertStatConnectInfor[Row].NextStatNo = 0;
        }
        else
        {
            for(Col = 0; Col < StationMaxCount; Col++)
            {
                if(!strcmp(XMLStatConnectInformation[Row].NextStatNo, ConvertStationNo[Col]))
                {
                    XMLConvertStatConnectInfor[Row].NextStatNo = Col+1;
                    break;
                }
            }
        }
    }


    //站点所属的线路
    int **StationLine = new int* [StationMaxCount];
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        StationLine[Row] = new int[4]();
    }

    for(Row = 0; Row < AllStationCount; Row++)
    {
        if(!StationLine[XMLConvertStatConnectInfor[Row].StatNo-1][0])
        {
            StationLine[XMLConvertStatConnectInfor[Row].StatNo-1][0] = XMLConvertStatConnectInfor[Row].LineNo;
        }
        else if(!StationLine[XMLConvertStatConnectInfor[Row].StatNo-1][1])
        {
            StationLine[XMLConvertStatConnectInfor[Row].StatNo-1][1] = XMLConvertStatConnectInfor[Row].LineNo;
        }
        else if(!StationLine[XMLConvertStatConnectInfor[Row].StatNo-1][2])
        {
            StationLine[XMLConvertStatConnectInfor[Row].StatNo-1][2] = XMLConvertStatConnectInfor[Row].LineNo;
        }
        else if(!StationLine[XMLConvertStatConnectInfor[Row].StatNo-1][3])
        {
            StationLine[XMLConvertStatConnectInfor[Row].StatNo-1][3] = XMLConvertStatConnectInfor[Row].LineNo;
        }
    }

//    cout<< sizeof(StationLine[0][0]);
//    cout<< sizeof(StationLine);
//    set<int> setPr(StationLine[19], StationLine[19]+sizeof(StationLine[0][0]));
//    cout<<*setPr.begin()<<'-'<<*setPr.end();

//    for (int i = 0; i < StationMaxCount; i++){
//        cout<<i+1<<':';
//        cout<<StationLine[i][0]<<' ';
//        cout<<StationLine[i][1]<<' ';
//        cout<<StationLine[i][2]<<' ';
//        cout<<StationLine[i][3]<<' ';
//        cout<<endl;
//    }




    /*车站是否换乘*/
    bool *Istrans = new bool[StationMaxCount];
    for(Row = 0; Row < AllStationCount; Row++)
    {
        Istrans[XMLConvertStatConnectInfor[Row].StatNo-1] = XMLConvertStatConnectInfor[Row].IsTrans;
    }

    /*车站间的站间距*/
    auto *StatDis = new StaDis[AllStationCount];
    for(Row = 0; Row < AllStationCount; Row++)
    {
        StatDis[Row].StationNum = XMLConvertStatConnectInfor[Row].StatNo;
        StatDis[Row].PreviStation = XMLConvertStatConnectInfor[Row].PreStatNo;
        StatDis[Row].PreviDis = XMLConvertStatConnectInfor[Row].PreStatDis;
        StatDis[Row].NextStation = XMLConvertStatConnectInfor[Row].NextStatNo;
        StatDis[Row].NextDis = XMLConvertStatConnectInfor[Row].NextStatDis;
        StatDis[Row].PreDirect = XMLConvertStatConnectInfor[Row].PreDirect;
        StatDis[Row].NextDirect = XMLConvertStatConnectInfor[Row].NextDirect;
    }


    /*车站停站时间*/
    auto *Statstoptime = new ODStatStopTime[AdjaceMaxCount];
    for(Row = 0, Col = 0; Row < AllStationCount && Col < AdjaceMaxCount; Row++)
    {
        if(XMLConvertStatConnectInfor[Row].PreStatNo != 0)
        {
            Statstoptime[Col].PreStationNo = XMLConvertStatConnectInfor[Row].StatNo;
            Statstoptime[Col].TOStationNo = XMLConvertStatConnectInfor[Row].PreStatNo;
            Statstoptime[Col].ToStaStopTime = XMLConvertStatConnectInfor[Row].StatStopTime1/60;
            Col++;
        }

        if(XMLConvertStatConnectInfor[Row].NextStatNo != 0)
        {
            Statstoptime[Col].PreStationNo = XMLConvertStatConnectInfor[Row].StatNo;
            Statstoptime[Col].TOStationNo = XMLConvertStatConnectInfor[Row].NextStatNo;
            Statstoptime[Col].ToStaStopTime = XMLConvertStatConnectInfor[Row].StatStopTime2/60;
            Col++;
        }
    }

    /*站间运行时间*/
    auto *StatTime = new StaTime[AllStationCount];
    for(Row = 0; Row < AllStationCount; Row++)
    {
        StatTime[Row].NextStation =  StatDis[Row].NextStation;
        StatTime[Row].NextTime =  StatDis[Row].NextDis/60;
        StatTime[Row].PreviStation =  StatDis[Row].PreviStation;
        StatTime[Row].PreviTime =  StatDis[Row].PreviDis/60;
        StatTime[Row].StationNum =  StatDis[Row].StationNum;
        StatTime[Row].PreDirect =  StatDis[Row].PreDirect;
        StatTime[Row].NextDirect =  StatDis[Row].NextDirect;
    }


    /*站点之间的连通关系,主要用于建立图形连接关系的邻接链表*/
    int **StationConnect = new int* [AdjaceMaxCount];
    for(Row = 0; Row < AdjaceMaxCount; Row++)
    {
        StationConnect[Row] = new int[2];
    }

    for(Row = 0, Col = 0; Row < AllStationCount && Col < AdjaceMaxCount; Row++)
    {
        if(XMLConvertStatConnectInfor[Row].PreStatNo != 0)
        {
            StationConnect[Col][0] = XMLConvertStatConnectInfor[Row].StatNo;
            StationConnect[Col][1] = XMLConvertStatConnectInfor[Row].PreStatNo;
            Col++;
        }

        if(XMLConvertStatConnectInfor[Row].NextStatNo != 0)
        {
            StationConnect[Col][0] = XMLConvertStatConnectInfor[Row].StatNo;
            StationConnect[Col][1] = XMLConvertStatConnectInfor[Row].NextStatNo;
            Col++;
        }
    }

    //OD点间票价
    auto **ODTicketPrice = new double* [StationMaxCount];
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        ODTicketPrice[Row] = new double[StationMaxCount]();
    }

    //共StationMaxCount * StationMaxCount个OD组合
    // 此循环只计算OD不为同一站点的情况(StationMaxCount*(StationMaxCount-1))个
//    for(Row = 0; Row < StationMaxCount*(StationMaxCount-1); Row++)
//    {
//        int OStation = 0;
//        int DStation = 0;
//        for(Index = 0; Index < StationMaxCount; Index++)
//        {
//            if(!strcmp(XMLODStatPrice[Row].Ori,ConvertStationNo[Index]))
//            {
//                OStation = Index+1;
//            }
//            if(!strcmp(XMLODStatPrice[Row].Des,ConvertStationNo[Index]))
//            {
//                DStation = Index+1;
//            }
//            if(OStation != 0 && DStation != 0)
//            {
//                ODTicketPrice[OStation-1][DStation-1] = XMLODStatPrice[Row].Price;
//                break;
//            }
//        }
//    }
//
//    //此循环计算OD为同一站点的情况
//    for(Row = 0; Row < StationMaxCount; Row++)
//    {
//        ODTicketPrice[Row][Row] = XMLLowestODPrice;
//    }

    /*换乘步行时间*/
    auto *TransStaWalkTim = new TransStaWalkTime[2*TransStationCount];
    for(Row = 0; Row < TransStationCount*2; Row++)
    {
        for(Col = 0; Col < StationMaxCount; Col++)
        {
            if(!strcmp(ConvertStationNo[Col], XMLTransfStatWalktime[Row].StationNo))
                TransStaWalkTim[Row].Stationnum = Col+1;
        }
        for(Col = 0; Col < LineCount; Col++)
        {
            if(!strcmp(XMLTransfStatWalktime[Row].FromLineNo, LineNoName[Col]))
                TransStaWalkTim[Row].Fromline = Col+1;
        }
        for(Col = 0; Col < LineCount; Col++)
        {
            if(!strcmp(XMLTransfStatWalktime[Row].ToLineNo, LineNoName[Col]))
                TransStaWalkTim[Row].Toline = Col+1;
        }
        TransStaWalkTim[Row].TranswalkTime[0][0] = XMLTransfStatWalktime[Row].TranswalkUpToUp;
        TransStaWalkTim[Row].TranswalkTime[0][1] = XMLTransfStatWalktime[Row].TranswalkUpToDown;
        TransStaWalkTim[Row].TranswalkTime[1][0] = XMLTransfStatWalktime[Row].TranswalkDownToUp;
        TransStaWalkTim[Row].TranswalkTime[1][1] = XMLTransfStatWalktime[Row].TranswalkDownToDown;
    }

    /*换乘时间*/
    auto *TransTime = new Transtime[2*TransStationCount];
    for(Row = 0; Row < TransStationCount*2; Row++)
    {
        TransTime[Row].Stationnum = TransStaWalkTim[Row].Stationnum;
        TransTime[Row].Fromline = TransStaWalkTim[Row].Fromline;
        TransTime[Row].Toline = TransStaWalkTim[Row].Toline;
        TransTime[Row].TransTime[0][0] = TransStaWalkTim[Row].TranswalkTime[0][0] + TrainInterval[TransTime[Row].Toline-1]/2;
        TransTime[Row].TransTime[0][1] = TransStaWalkTim[Row].TranswalkTime[0][1] + TrainInterval[TransTime[Row].Toline-1]/2;
        TransTime[Row].TransTime[1][0] = TransStaWalkTim[Row].TranswalkTime[1][0] + TrainInterval[TransTime[Row].Toline-1]/2;
        TransTime[Row].TransTime[1][1] = TransStaWalkTim[Row].TranswalkTime[1][1] + TrainInterval[TransTime[Row].Toline-1]/2;
    }

    /*OD点之间的最短时间            */
    auto **StaShortTime = new double* [StationMaxCount];
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        StaShortTime[Row] = new double[StationMaxCount];
    }

    /*站点邻接链表头*/
    auto *Head = new struct MetroStation[StationMaxCount];

    /*搜索路径的记录站点情况        */
    auto *SearchQueue = new SearStation[StationMaxCount];

    /*记录搜索过程中各站之间的时间*/
    tempRecordsOfTime = new double[StationMaxCount]();

    /*用于节点是否被访问的标记*/
    int *Visited = new int[StationMaxCount+1];

    //删除储存XML配置信息的空间（开始）
    delete []XMLTrainInterval;
    delete []XMLSpeed;
    for(XMLindex = 0; XMLindex < XMLlineCount; XMLindex++)
    {
        delete []XMLLineNoName[XMLindex];
    }
    delete []XMLLineNoName;
    delete []XMLStatConnectInformation;
    delete []XMLTransfStatWalktime;
    delete []XMLODStatPrice;
    delete myDocument;
    //删除储存XML配置信息的空间（结束）

    //初始化头节点和访问队列
    Visited[0] = 1;
    for(Index = 0; Index < StationMaxCount; Index++)
    {
        Head[Index].vertex = Index+1;
        Head[Index].NextStation = nullptr;
        Visited[Index+1] = 0;
    }

    Creategraph(StationConnect, AdjaceMaxCount, Head);              //建立路网拓朴图形

//    //打开储存最短路径时间信息的文件
//    if((ShortTimeFormat = fopen(GetShortTime,"w+")) == nullptr)
//    {
//        printf("GetShortTime can not open.\n");
//        return 3;
//    }

    //打开储存最短路径的文件
    if((ShortPathfp = fopen(ShortPathFile,"w+")) == nullptr)
    {
        cout<<"ShortPathfp can not open"<<endl;
        return 3;
    }

    //查找最短路径
    for(Index = 1; Index <= StationMaxCount; Index++)
        Dijkstra(Index, StationMaxCount, Head, Istrans, AllStationCount, AdjaceMaxCount, StatTime,
                 Statstoptime,StationLine, TransStationCount, TransTime, StaShortTime);

    //关闭最短路径文件
    fclose(ShortPathfp);

    //打开储存所有路径的文件
    if((AllPathfp = fopen(AllPathFile,"w+")) == nullptr)
    {
        printf("AllPathFile can not open.\n");
        return 3;
    }

    //用于遍历起始和结束节点的路径
    for(int src = 1; src <= StationMaxCount; src++)
    {
        for(int des = 1; des <= StationMaxCount; des++)
        {
            memset(Visited, 0, sizeof(*Visited)*(StationMaxCount+1));
            SearchPth(src, des ,StationMaxCount, SearchQueue, Visited, Head,
                      Istrans, AllStationCount, AdjaceMaxCount, StatTime, Statstoptime,StationLine, TransStationCount,
                      TransTime, StaShortTime);
        }
    }

    cout<<NotValidCount<<endl;
    cout<< ChangeTimeCount <<endl;
    fclose(AllPathfp);

    /******************************/
    /*              结束           */
    /******************************/
    DeleteGraph(StationMaxCount, Head);            //删除邻接列表

    delete []TrainInterval;            //删除列车发车间隔数组空间

    delete []Speed;                    //删除列车速度数组空间

    for(Row = 0; Row < LineCount; Row++)  //线路名称
    {
        delete []LineNoName[Row];
    }
    delete []LineNoName;

    //定义站点编号数组用于转换成清分时定义的站点编号
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        delete []ConvertStationNo[Row];
    }
    delete []ConvertStationNo;

    //站点编号数组
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        delete []StationName[Row];
    }
    delete []StationName;


    //把XML数据根据车站编号、线路编号、清分线路编号的序列号进行转换
    delete []XMLConvertStatConnectInfor;

    //站点所属的线路
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        delete []StationLine[Row];
    }
    delete []StationLine;

    delete []Istrans;         //车站是否换乘

    delete []StatDis;         //车站间的站间距


    delete []Statstoptime;        //车站停站时间

    delete []StatTime;         //站间运行时间

    //站点之间的连通关系,主要用于建立图形连接关系的邻接链表
    for(Row = 0; Row < AdjaceMaxCount; Row++)
    {
        delete []StationConnect[Row];
    }
    delete []StationConnect;

    //OD点间票价
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        delete []ODTicketPrice[Row];
    }
    delete []ODTicketPrice;

    //换乘步行时间
    delete []TransStaWalkTim;

    //换乘时间
    delete []TransTime;

    //OD点之间的最短时间
    for(Row = 0; Row < StationMaxCount; Row++)
    {
        delete []StaShortTime[Row];
    }
    delete []StaShortTime;

    delete []Head;    //站点邻接链表头

    delete []SearchQueue;   //搜索路径的记录站点情况

    delete []Visited;       //用于节点是否被访问的标记

//    fclose(ShortTimeFormat);

    for(Row = 0; Row < StationMaxCount; Row++)
    {
        delete []CopyStationName[Row];
    }
    delete []CopyStationName;

    return 4;
}