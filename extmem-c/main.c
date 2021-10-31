#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "extmem.h"

typedef struct tuple{
int x;
int y;
}tuple;
//任务一：基于线性搜索的关系选择算法
void linear_select();

//任务二:两阶段多路归并排序
void TPMMS();

//任务三：基于索引的关系选择算法
void create_index(int first_sorted_blk, int last_sorted_blk, int first_index_blk, int *last_index_blk);
void index_search(int first_index_blk, int last_index_blk, int first_output_blk, int X);

//任务四：基于排序的连续操作算法
void sort_merge_join(int R_first_sorted_blk, int R_last_sorted_blk, int S_first_sorted_blk, int S_last_sorted_blk,
                     int first_output_blk);

//任务五：基于排序的两趟扫描算法,交
void sort_merge_intersection(int R_first_sorted_blk, int R_last_sorted_blk, int S_first_sorted_blk, int S_last_sorted_blk,
                        int first_output_blk);
void write_address_to_block(unsigned char *blk, int address);
void write_tuple_to_block(unsigned char *blk, int offset, tuple t);
void write_attr_to_block(unsigned char *blk, int offset, int attr);
void write_tuple_to_block_and_disk(Buffer buf, unsigned char **blk, int *addr, int *offset, tuple t);

int main()
{
    int last_index;
    //线性搜索
    linear_select();
    //归并排序
    TPMMS();
    //创建索引
    create_index(2017,2048,301,&last_index);
    //基于索引排序的搜索
    index_search(301,last_index,310,130);
    //基于排序的连接
    sort_merge_join(2001,2016,2017,2048,401);
    //基于排序的交
    sort_merge_intersection(2001,2016,2017,2048,5001);
    return 0;
}
//得到一个元组的值
tuple getvalue(unsigned char *p, int offset) {
    tuple t;
    // 属性 X
    if(offset==-1)
    {
        t.x = 1000000;
        t.y = 1000000;
        return t;
    }
    char str[5] = "";
    for (int i = 0; i < 4; i++) {
        str[i] = (char) *(p + offset * 8 + i);
    }
    t.x = atoi(str);

    // 属性 Y
    for (int i = 0; i < 4; i++) {
        str[i] = (char) *(p + offset * 8 + 4 + i);
    }
    t.y = atoi(str);

    return t;
}

void linear_select()
{
    int X = -1;
    int Y = -1;
    int count = 0;
    char str[5];
    Buffer buf; /* A buffer */
    unsigned char *blkr, *blkw, *begin; /* A pointer to a block */

    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return;
    }

    printf("\n\n-------------------------------\n"
           "基于线性选搜索的关系选择算法\n"
           "-------------------------------\n");
    /* Get a new block in the buffer */
    blkw = getNewBlockInBuffer(&buf);
    begin = blkw;
    for(int j=17; j<49; j++)
    {

        if ((blkr = readBlockFromDisk(j, &buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return ;
        }
        printf("读入数据块%d\n", j);

        /* Process the data in the block */
        for (int i = 0; i < 7; i++) //一个blk存7个元组加一个地址
        {

            for (int k = 0; k < 4; k++)
            {
                str[k] = *(blkr + i*8 + k);
            }
            X = atoi(str);
            for (int k = 0; k < 4; k++)
            {
                str[k] = *(blkr + i*8 + 4 + k);
            }
            Y = atoi(str);
            if(X == 130)
            {
                count++;
                printf("(%d, %d)\n", X, Y);
                if(blkw > (begin+64))
                {
                    if (writeBlockToDisk(begin, 1111, &buf) != 0)
                    {
                        perror("Writing Block Failed!\n");
                        return ;
                    }
                    freeBlockInBuffer(blkw, &buf);
                    blkw = getNewBlockInBuffer(&buf);
                    begin = blkw;
                }
                *blkw = (char)('1');
                *(blkw+1) = (char)('3');
                *(blkw+2) = (char)('0');
                for(int m=4; m<8; m++)
                {
                    *(blkw+m)=(char)str[m-4];
                }
                blkw+=8;
            }

        }
        freeBlockInBuffer(blkr, &buf);
    }
    if (writeBlockToDisk(begin, 1111, &buf) != 0)
    {
        perror("Writing Block Failed!\n");
        return ;
    }
    printf("注：结果写入磁盘：1111\n");
    printf("一共找到%d个元组\n", count);
    printf("IO读写一共%d次\n",buf.numIO);
}

//交换buffer中的两个元组
void exchange(unsigned char* p1, unsigned char* p2)
{
    char temp;
    for(int i=0; i<8; i++)
    {
        temp = *(p1+i);
        *(p1+i) = *(p2+i);
        *(p2+i) = temp;
    }
}

//排序
int sortdata(Buffer* buf, int addr)
{
    unsigned char* data = buf->data;

    for(int i=0; i<55; i++)
    {
        int a = i/7;
        int b = i%7;
        for(int j=i+1; j<56; j++)
        {
            int c = j/7;
            int d = j%7;
            tuple t1 = getvalue(data+65*a+1, b);
            tuple t2 = getvalue(data+65*c+1, d);
            if(t1.x>t2.x || (t1.x == t2.x && t1.y > t2.y))
                exchange(data+65*a+b*8+1, data+65*c+d*8+1);
        }
    }
    for(int i=0; i<8; i++)
        writeBlockToDisk(data+i*65+1, addr+i, buf);

    return 1;
}


//R归并第一阶段
void Rset(Buffer* buf)
{
    unsigned char *blk; /* A pointer to a block */
    /* Initialize the buffer */

    // 112元组，分成2个集合，每个56个元素，8各组，装满buffer
    for(int i=1; i<9; i++) // 获取第一个集合
    {
        if ((blk = readBlockFromDisk(i, buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return ;
        }
    }
    sortdata(buf, 211);

    for(int i=9; i<17; i++) // 获取第一个集合
    {
        if ((blk = readBlockFromDisk(i, buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return ;
        }
    }
    sortdata(buf, 219);
    for(int i=211; i<227; i++)
        printf("R归并第一阶段结果写入磁盘%d\n",i);
}


//S归并第一阶段
void Sset(Buffer* buf)
{
    unsigned char *blk; /* A pointer to a block */
    /* Initialize the buffer */

    // 224元组，分成4个集合，每个56个元素，8各组，装满buffer
    for(int i=17; i<25; i++) // 获取第一个集合
    {
        if ((blk = readBlockFromDisk(i, buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return ;
        }
    }
    sortdata(buf, 227);

    for(int i=25; i<33; i++) // 获取第二个集合
    {
        if ((blk = readBlockFromDisk(i, buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return ;
        }
    }
    sortdata(buf, 235);

    for(int i=33; i<41; i++) // 获取第三个集合
    {
        if ((blk = readBlockFromDisk(i, buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return ;
        }
    }
    sortdata(buf, 243);

    for(int i=41; i<49; i++) // 获取第四个集合
    {
        if ((blk = readBlockFromDisk(i, buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return ;
        }
    }
    sortdata(buf, 251);
    for(int i=227; i<259; i++)
        printf("S归并第一阶段结果写入磁盘%d\n",i);
}


void printtest(Buffer* buf)
{
    unsigned char* p = buf->data;
    for(int i=0; i<56; i++)
    {
        int a = i/7;
        int b = i%7;
        int X = -1;
        int Y = -1;
        char str[5];
        for (int k = 0; k < 4; k++)
        {
            str[k] = *(p + a * 65 + b*8 + 1 + k);
        }
        X = atoi(str);
        for (int k = 0; k < 4; k++)
        {
            str[k] = *(p + a * 65 + b * 8+ 5 + k);
        }
        Y = atoi(str);
        printf("(%d,%d)\n", X,Y);
    }
}

//R的归并排序
void TPMMSR()
{
    Buffer buf; /* A buffer */
    unsigned char *blkw; /* A pointer to a block */
    unsigned char *rblkr[2];
    int a,b,wcount,i,j, addr;
    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return ;
    }
    Rset(&buf);
    blkw = getNewBlockInBuffer(&buf);//用于写磁盘
    rblkr[0] = readBlockFromDisk(211, &buf);
    rblkr[1] = readBlockFromDisk(219, &buf);//两路归并
    wcount = 0;
    addr = 0;
    a = 0;
    b = 0;
    for(i=212, j=220; i<219, j<227; )
    {
        if(a == 7)//当前块用完，取出集合的下一块
        {
            freeBlockInBuffer(rblkr[0], &buf);
            rblkr[0] = readBlockFromDisk(i, &buf);
            i++;
            a=0;
        }
        if(b == 7)
        {
            freeBlockInBuffer(rblkr[1], &buf);
            rblkr[1] = readBlockFromDisk(j, &buf);
            j++;
            b=0;
        }
        while(a<7 && b<7)
        {
            if(wcount == 7)//写块满了就写入磁盘
            {
                writeBlockToDisk(blkw, 2001+addr, &buf);
                blkw = getNewBlockInBuffer(&buf);
                wcount = 0;
                addr++;
            }
            tuple x0 = getvalue(rblkr[0],a);
            tuple x1 = getvalue(rblkr[1],b);
            if(x0.x<x1.x || (x0.x == x1.x && x0.y < x1.y))
            {
                exchange(rblkr[0]+a*8, blkw+wcount*8);
                wcount++;
                a++;
            }
            else
            {
                exchange(rblkr[1]+b*8, blkw+wcount*8);
                wcount++;
                b++;
            }
        }
    }
    if(i==219)//处理剩一个集合未写入的情况
    {
        while(b < 7)
        {
            exchange(rblkr[1]+8*b, blkw+wcount*8);
            b++;
        }
        freeBlockInBuffer(rblkr[1], &buf);
        writeBlockToDisk(blkw, 2001+addr, &buf);
        addr++;
        wcount = 0;
        for(int k=j; k<227; k++)
        {
            rblkr[1] = readBlockFromDisk(k, &buf);
            writeBlockToDisk(rblkr[1], 2001+addr, &buf);
            addr++;
        }
    }
    else
    {
        while(a < 7)
        {
            exchange(rblkr[0]+8*a, blkw+wcount*8);
            a++;
        }
        freeBlockInBuffer(rblkr[0], &buf);
        writeBlockToDisk(blkw, 2001+addr, &buf);
        addr++;
        wcount = 0;
        for(int k=i; k<219; k++)
        {
            rblkr[0] = readBlockFromDisk(k, &buf);
            writeBlockToDisk(rblkr[0], 2001+addr, &buf);
            addr++;
        }
    }
    for(int i=2001; i< 2017; i++)
        printf("R排序结果写入磁盘%d\n",i);
    printf("R的IO次数：%d\n\n", buf.numIO);
}

//找到数组中最小的
int getsmall(tuple a[4])
{
    if(a[0].x==1000000 && a[1].x==1000000 && a[2].x==1000000 && a[3].x==1000000)
        return -1;//等于1000000意味着当前集合所有块都已写入磁盘，不会再作为最小值返回
    tuple t;
    t.x = 10000;
    t.y = 10000;
    int index;
    for(int i=0; i<4; i++)
    {
        if(a[i].x<t.x || (a[i].x == t.x && a[i].y < t.y))
        {
            t = a[i];
            index = i;
        }
    }
    return index;
}


//S归并排序
void TPMMSS()
{
    Buffer buf; /* A buffer */
    unsigned char *blkw; /* A pointer to a block */
    unsigned char *rblkr[4];
    int wcount, addr;
    int a[4] = {0,0,0,0};
    int set1=0, set2=0, set3=0, set4=0, flag=1;
    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return ;
    }
    Sset(&buf);
    blkw = getNewBlockInBuffer(&buf);
    rblkr[0] = readBlockFromDisk(227, &buf);
    rblkr[1] = readBlockFromDisk(235, &buf);
    rblkr[2] = readBlockFromDisk(243, &buf);
    rblkr[3] = readBlockFromDisk(251, &buf);
    set1=228, set2=236, set3=244, set4=252;
    wcount = 0;
    addr = 0;
    while(flag)
    {
        if(a[0] == 7)
        {
            if(set1<235)
            {
                freeBlockInBuffer(rblkr[0], &buf);
                rblkr[0] = readBlockFromDisk(set1, &buf);
                set1++;
                a[0]=0;
            }
            else
                a[0] = -1;
        }
        if(a[1] == 7)
        {
            if(set2<243)
            {
                freeBlockInBuffer(rblkr[1], &buf);
                rblkr[1] = readBlockFromDisk(set2, &buf);
                set2++;
                a[1]=0;
            }
            else
                a[1]=-1;
        }
        if(a[2] == 7)
        {
            if(set3<251)
            {
                freeBlockInBuffer(rblkr[2], &buf);
                rblkr[2] = readBlockFromDisk(set3, &buf);
                set3++;
                a[2]=0;
            }
            else
                a[2]=-1;
        }
        if(a[3] == 7)
        {
            if(set4<259)
            {
                freeBlockInBuffer(rblkr[3], &buf);
                rblkr[3] = readBlockFromDisk(set4, &buf);
                set4++;
                a[3]=0;
            }
            else
                a[3]=-1;
        }
        while(a[0]<7 && a[1]<7 && a[2]<7 && a[3]<7)
        {
            if(wcount == 7)
            {
                writeBlockToDisk(blkw, 2017+addr, &buf);
                blkw = getNewBlockInBuffer(&buf);
                wcount = 0;
                addr++;
            }
            tuple x0 = getvalue(rblkr[0],a[0]);
            tuple x1 = getvalue(rblkr[1],a[1]);
            tuple x2 = getvalue(rblkr[2],a[2]);
            tuple x3 = getvalue(rblkr[3],a[3]);
            tuple x[4] = {x0, x1, x2, x3};
            int res = getsmall(x);
            if(res == -1)
            {
                flag=0;
                break;
            }
            else
            {
                exchange(rblkr[res]+a[res]*8, blkw+wcount*8);
                wcount++;
                a[res]++;
            }
        }
    }
    for(int i=2017; i< 2049; i++)
        printf("S排序结果写入磁盘%d\n", i);
    printf("S的IO次数：%d\n\n", buf.numIO);
}


//R、S归并排序
void TPMMS()
{
    printf("\n\n-------------------------------\n"
           "两阶段多路归并排序\n"
           "-------------------------------\n");
    TPMMSR();
    TPMMSS();
}


void create_index(int first_sorted_blk, int last_sorted_blk, int first_index_blk, int *last_index_blk) {

    Buffer buf; /* A buffer */
    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    printf("\n\n-------------------------------\n"
           "对关系 S 创建索引\n"
           "-------------------------------\n");

    int cur_output_blk = first_index_blk;   // 当前输出块的编号
    int offset = 0;                         // 输出块中偏移量

    // 输出用的索引块
    unsigned char *index_blk;
    if ((index_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    // 逐个遍历
    for (int addr = first_sorted_blk; addr <= last_sorted_blk; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        // 记录每个块第一个元组的第一个属性
        tuple index = getvalue(blk, 0);
        index.y = addr;
        write_tuple_to_block_and_disk(buf,&index_blk, &cur_output_blk, &offset, index);

        freeBlockInBuffer(blk, &buf);
    }

    *last_index_blk = cur_output_blk;   // 记录最后一个索引块的编号

    // 缓冲区中仍有未写回磁盘的数据
    if (offset != 0) {
        // 写入下一块的地址
        write_address_to_block(index_blk, cur_output_blk + 1);

        // 写入磁盘
        if (writeBlockToDisk(index_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        // 释放
        freeBlockInBuffer(index_blk, &buf);
    }
}

void write_attr_to_block(unsigned char *blk, int offset, int attr) {
    char str_attr[4 + 1] = "";
    sprintf(str_attr, "%d", attr);
    memcpy(blk + offset * 4, str_attr, 4);
}

void write_address_to_block(unsigned char *blk, int address) {
    char str_addr[8 + 1] = "";
    sprintf(str_addr, "%d", address);
    memcpy(blk + 8 * 7, str_addr, 8);
}

void write_tuple_to_block(unsigned char *blk, int offset, tuple t) {
    write_attr_to_block(blk, offset * 2, t.x);
    write_attr_to_block(blk, offset * 2 + 1, t.y);
}

void write_join_tuple_to_block(unsigned char *blk, int offset, tuple t1, tuple t2) {
    write_tuple_to_block(blk, offset * 2, t1);
    write_tuple_to_block(blk, offset * 2 + 1, t2);
}


void write_join_tuple_to_block_and_disk(Buffer buf,unsigned char **blk, int *addr, int *offset, tuple t1, tuple t2) {
    if (*offset >= 3) {
        // 当前块已满，写入磁盘
        write_address_to_block(*blk, *addr + 1);

        // 写入磁盘
        if (writeBlockToDisk(*blk, *addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", *addr);

        (*addr)++;                          // 更新磁盘块编号
        *blk = getNewBlockInBuffer(&buf);   // 重新申请 block
        memset(*blk, 0, 64);        // 清空
        *offset = 0;                        // 重置输出位置
    }

    write_join_tuple_to_block(*blk, *offset, t1, t2);
    (*offset)++;
}


void write_tuple_to_block_and_disk(Buffer buf, unsigned char **blk, int *addr, int *offset, tuple t) {
    if (*offset >= 7) {
        // 当前块已满，写入磁盘
        write_address_to_block(*blk, *addr + 1);

        // 写入磁盘
        if (writeBlockToDisk(*blk, *addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", *addr);

        (*addr)++;                          // 更新磁盘块编号
        *blk = getNewBlockInBuffer(&buf);   // 重新申请 block
        *offset = 0;                        // 重置输出位置
    }

    write_tuple_to_block(*blk, *offset, t);
    (*offset)++;
}



void index_search(int first_index_blk, int last_index_blk, int first_output_blk, int X) {

    Buffer buf; /* A buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    char rel[4];
    if (first_index_blk != 301) {
        strcpy(rel, "R.A");
    } else {
        strcpy(rel, "S.C");
    }

    printf("\n\n-------------------------------\n"
           "基于索引的选择算法 %s=%d:\n"
           "-------------------------------\n", rel, X);

    int cur_output_blk = first_output_blk;  // 当前输出块的编号
    int offset = 0;                         // 输出块中的偏移量
    int cnt = 0;                            // 满足条件的元组数

    int index_find = 0;    // 记录是否找到所需的索引
    int blk_index = 0;          // 所需读取的首个数据块
    // 遍历索引块
    for (int addr = first_index_blk; addr <= last_index_blk && !index_find; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入索引块 %d\n", addr);

        // 遍历块中每个索引
        for (int i = 0; i < 7; i++) {
            tuple index = getvalue(blk, i);
            if (index.x >= X) {
                // 不小于所需值，需要从前一个索引开始搜索
                index_find = 1;
                break;
            }
            blk_index = index.y;
        }

        // 释放
        freeBlockInBuffer(blk, &buf);
    }
    if (!index_find) {
        printf("没有满足条件的元组。\n");
        return;
    }
    unsigned char *result_blk;      // 储存输出结果
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    int finish_flag = 0;       // 记录是否搜索完毕
    // 从指定位置开始遍历数据块
    for (int addr = blk_index; addr <= 2048 && !finish_flag; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入数据块 %d\n", addr);

        // 遍历块中每个元组
        for (int i = 0; i < 7; i++) {
            tuple t = getvalue(blk, i);
            if (t.x == X) {
                // 符合条件
                write_tuple_to_block_and_disk(buf, &result_blk, &cur_output_blk, &offset, t);
                printf("(X=%d, Y=%d)\n", t.x, t.y);
                cnt++;
            } else if (t.x > X) {
                // 搜索完毕
                finish_flag = 1;
                break;
            }
        }

        // 释放
        freeBlockInBuffer(blk, &buf);
    }

    // 缓冲区中仍有未写回磁盘的数据
    if (offset != 0) {
        // 写入下一块的地址
        write_address_to_block(result_blk, cur_output_blk + 1);

        // 写入磁盘
        if (writeBlockToDisk(result_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        // 释放
        freeBlockInBuffer(result_blk, &buf);
    }

    printf("\n满足选择条件的元组一共 %d 个。\n\n"
           "IO读写一共 %d 次。\n", cnt, buf.numIO);
}

void sort_merge_join(int R_first_sorted_blk, int R_last_sorted_blk, int S_first_sorted_blk, int S_last_sorted_blk,
                     int first_output_blk) {
    printf("\n\n-------------------------------\n"
           "基于排序的连接算法\n"
           "-------------------------------\n");


    Buffer buf; /* A buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    int join_cnt = 0;                       // 连接次数

    int r_cur_blk = R_first_sorted_blk;     // 当前读取的 R 关系的块
    int s_cur_blk = S_first_sorted_blk;     // 当前读取的 S 关系的块

    int cur_output_blk = first_output_blk;  // 当前输出块的编号
    int offset = 0;                         // 输出块中的偏移量

    // 初始化
    unsigned char *result_blk;              // 记录输出结果
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    memset(result_blk, 0, 64);        // 清空

    unsigned char *r_blk;                   // R
    if ((r_blk = readBlockFromDisk(r_cur_blk, &buf)) == NULL) {
        perror("Reading Block Failed!\n");
        exit(-1);
    }

    unsigned char *s_blk;                   // S
    if ((s_blk = readBlockFromDisk(s_cur_blk, &buf)) == NULL) {
        perror("Reading Block Failed!\n");
        exit(-1);
    }

    int r_blk_offset = 0;                   // 记录当前正在读取的 R 的元组在块中的偏移量
    int s_blk_offset = 0;                   // 记录当前正在读取的 S 的元组在块中的偏移量

    while (r_cur_blk <= R_last_sorted_blk && s_cur_blk <= S_last_sorted_blk) {
        tuple t_r = getvalue(r_blk, r_blk_offset);
        tuple t_s = getvalue(s_blk, s_blk_offset);

        if (t_r.x < t_s.x) {
            r_blk_offset++;
        } else if (t_r.x > t_s.x) {
            s_blk_offset++;
        } else {
            // t_r.x == t_s.x
            // 遍历从此处开始的 R 关系的元组，直到 t_r.x > t_s.x
            int r_temp_blk = r_cur_blk;
            int r_temp_offset = r_blk_offset;

            unsigned char *r_temp;
            if ((r_temp = readBlockFromDisk(r_temp_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }

            // 从当前位置开始遍历，记录所有连接后的元组，直到 t_r.x > t_s.x 或遍历完所有块
            while (r_temp_blk <= R_last_sorted_blk) {
                tuple t_r_temp = getvalue(r_temp, r_temp_offset);

                if (t_r_temp.x > t_s.x) {
                    freeBlockInBuffer(r_temp, &buf);
                    break;
                }

                write_join_tuple_to_block_and_disk(buf, &result_blk, &cur_output_blk, &offset, t_r_temp, t_s);
                join_cnt++;

                r_temp_offset++;    // 下一个元组
                if (r_temp_offset >= 7) {
                    // 当前块已读完
                    // 释放块
                    freeBlockInBuffer(r_temp, &buf);
                    r_temp_blk++;   // 下一块
                    if (r_temp_blk > R_last_sorted_blk) {
                        // 遍历完
                        break;
                    }
                    if ((r_temp = readBlockFromDisk(r_temp_blk, &buf)) == NULL) {
                        perror("Reading Block Failed!\n");
                        exit(-1);
                    }
                    r_temp_offset = 0;  // 重置偏移量
                }
            }

            s_blk_offset++;     // 下一个元组
        }

        if (r_blk_offset >= 7) {
            // 当前块已读完
            freeBlockInBuffer(r_blk, &buf);
            r_cur_blk++;
            if (r_cur_blk > R_last_sorted_blk) {
                // 遍历完
                break;
            }
            if ((r_blk = readBlockFromDisk(r_cur_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }
            r_blk_offset = 0;
        }

        if (s_blk_offset >= 7) {
            // 当前块已读完
            freeBlockInBuffer(s_blk, &buf);
            s_cur_blk++;
            if (s_cur_blk > S_last_sorted_blk) {
                // 遍历完
                break;
            }
            if ((s_blk = readBlockFromDisk(s_cur_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }
            s_blk_offset = 0;
        }
    }

    // 缓冲区中仍有未写回磁盘的数据
    if (offset != 0) {
        // 当前块已满，写入磁盘
        // 写地址
        write_address_to_block(result_blk, cur_output_blk + 1);

        // 写入磁盘
        if (writeBlockToDisk(result_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        // 释放
        freeBlockInBuffer(result_blk, &buf);
    }

    // 释放
    freeBlockInBuffer(r_blk, &buf);
    freeBlockInBuffer(s_blk, &buf);

    printf("\n总共连接 %d 次。\n", join_cnt);
}

void
sort_merge_intersection(int Rbegin, int Rend, int Sbegin, int Send,
                        int outputbegin) {
    printf("\n\n-------------------------------\n"
           "基于排序的交算法\n"
           "-------------------------------\n");

    int cnt = 0;                            // 符合条件的元组数

    Buffer buf; /* A buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    int r_cur_blk = Rbegin;     // 当前读取的 R 关系的块
    int s_cur_blk = Sbegin;     // 当前读取的 S 关系的块

    int cur_output_blk = outputbegin;  // 当前输出块的编号
    int offset = 0;                         // 输出块中的偏移量

    // 初始化
    unsigned char *result_blk;              // 记录输出结果
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    memset(result_blk, 0, 64);        // 清空

    unsigned char *r_blk;                   // R
    if ((r_blk = readBlockFromDisk(r_cur_blk, &buf)) == NULL) {
        perror("Reading Block Failed!\n");
        exit(-1);
    }

    unsigned char *s_blk;                   // S
    if ((s_blk = readBlockFromDisk(s_cur_blk, &buf)) == NULL) {
        perror("Reading Block Failed!\n");
        exit(-1);
    }

    int r_blk_offset = 0;                   // 记录当前正在读取的 R 的元组在块中的偏移量
    int s_blk_offset = 0;                   // 记录当前正在读取的 S 的元组在块中的偏移量

    while (r_cur_blk <= Rend && s_cur_blk <= Send) {
        tuple t_r = getvalue(r_blk, r_blk_offset);
        tuple t_s = getvalue(s_blk, s_blk_offset);

        if (t_r.x < t_s.x) {
            r_blk_offset++;
        } else if (t_r.x > t_s.x) {
            s_blk_offset++;
        } else {
            // t_r.x == t_s.x
            // 遍历从此处开始的 R 关系的元组，直到 t_r.x > t_s.x
            int r_temp_blk = r_cur_blk;
            int r_temp_offset = r_blk_offset;

            unsigned char *r_temp;
            if ((r_temp = readBlockFromDisk(r_temp_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }

            // 从当前位置开始遍历，记录所有相同的元组，直到 t_r.x > t_s.x 或遍历完所有块
            while (r_temp_blk <= Rend) {
                tuple t_r_temp = getvalue(r_temp, r_temp_offset);

                if (t_r_temp.x > t_s.x) {
                    freeBlockInBuffer(r_temp, &buf);
                    break;
                }

                if (t_r_temp.y == t_s.y) {
                    // 元组相同
                    write_tuple_to_block_and_disk(buf,&result_blk, &cur_output_blk, &offset, t_s);
                    printf("(X=%d, Y=%d)\n", t_s.x, t_s.y);
                    cnt++;
                }

                r_temp_offset++;    // 下一个元组
                if (r_temp_offset >= 7) {
                    // 当前块已读完
                    // 释放块
                    freeBlockInBuffer(r_temp, &buf);
                    r_temp_blk++;   // 下一块
                    if (r_temp_blk > Rend) {
                        // 遍历完
                        break;
                    }
                    if ((r_temp = readBlockFromDisk(r_temp_blk, &buf)) == NULL) {
                        perror("Reading Block Failed!\n");
                        exit(-1);
                    }
                    r_temp_offset = 0;  // 重置
                }
            }

            s_blk_offset++;     // 下一个元组
        }

        if (r_blk_offset >= 7) {
            // 当前块已读完
            freeBlockInBuffer(r_blk, &buf);
            r_cur_blk++;    // 下一块
            if (r_cur_blk > Rend) {
                // 遍历完
                break;
            }
            if ((r_blk = readBlockFromDisk(r_cur_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }
            r_blk_offset = 0;
        }

        if (s_blk_offset >= 7) {
            // 当前块已读完
            freeBlockInBuffer(s_blk, &buf);
            s_cur_blk++;
            if (s_cur_blk > Send) {
                // 遍历完
                break;
            }
            if ((s_blk = readBlockFromDisk(s_cur_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }
            s_blk_offset = 0;
        }
    }

    if (offset != 0) {
        // 当前块已满，写入磁盘
        // 写地址
        write_address_to_block(result_blk, cur_output_blk + 1);

        // 写入磁盘
        if (writeBlockToDisk(result_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        // 释放
        freeBlockInBuffer(result_blk, &buf);
    }

    // 释放
    freeBlockInBuffer(r_blk, &buf);
    freeBlockInBuffer(s_blk, &buf);

    printf("\nS 和 R 的交集有 %d 个元组。\n", cnt);
}

