#include "swap.h"

static void AddAssets(MARKET_DATA*);
static void DeleteAssets(MARKET_DATA*);
static void PrintRateOfChange(MARKET_DATA*,boolean*);
static void DeleteOldData(MARKET_DATA*);
static double AssetTimeIntegral(MARKET_DATA*,int,int,int);

extern double GetLeastSquare(
	MARKET_DATA *data,
	int m,
	int n,
	double *Q,
	double *L)
{
	int i,is,N;
	double *value;
	double *time;
	double QA,QB,QC;
	double LA,LB;
	FILE *xfile;
	char fname[100];
	int id = (n-1)/4;
	double base = data->base[m];

	is = (data->num_segs < n) ? 0 : data->num_segs - n;
	N = data->num_segs - is;
	value = data->assets[m].norm_price + is;
	FT_VectorMemoryAlloc((POINTER*)&time,N,sizeof(double));
	for (i = 0; i < N; ++i) time[i] = i*0.25;
	LeastSquareLinear(time,value,N,&LA,&LB);
	sprintf(fname,"xg/Fit-l-%d-%d.xg",m,id);
	xfile = fopen(fname,"w");
	fprintf(xfile,"color=green\n");
	fprintf(xfile,"thickness = 1.5\n");
	for (i = 0; i < N; ++i)
	{
	    fprintf(xfile,"%f %f\n",time[i],value[i]);
	}
	fprintf(xfile,"Next\n");
	fprintf(xfile,"color=red\n");
	fprintf(xfile,"thickness = 1.5\n");
	for (i = 0; i < N; ++i)
	{
	    fprintf(xfile,"%f %f\n",time[i],LA*time[i]+LB);
	}
	fclose(xfile);
	LeastSquareQuadr(time,value,N,&QA,&QB,&QC);
	sprintf(fname,"xg/Fit-q-%d-%d.xg",m,id);
	xfile = fopen(fname,"w");
	fprintf(xfile,"color=green\n");
	fprintf(xfile,"thickness = 1.5\n");
	for (i = 0; i < N; ++i)
	{
	    fprintf(xfile,"%f %f\n",time[i],value[i]);
	}
	fprintf(xfile,"Next\n");
	fprintf(xfile,"color=red\n");
	fprintf(xfile,"thickness = 1.5\n");
	for (i = 0; i < N; ++i)
	{
	    fprintf(xfile,"%f %f\n",time[i],QA*sqr(time[i])+QB*time[i]+QC);
	}
	fclose(xfile);
	FT_FreeThese(1,time);
	Q[0] = QA;	Q[1] = QB;	Q[2] = QC;
	L[0] = LA;	L[1] = LB;
}	/* end GetLeastSquare */

extern void LeastSquareQuadr(
	double *x,
	double *y,
	int N,
	double *A,
	double *B,
	double *C)
{
	double x0,x1,x2,x3,x4,yx0,yx1,yx2;
	double sx0,sx1,sx2,sx3,sx4,syx0,syx1,syx2;
	double a[3],b[3],c[3],d[3];
	double dinom;
	int i;

	x0 = x1 = x2 = x3 = x4 = yx0 = yx1 = yx2 = 0.0;
	sx0 = sx1 = sx2 = sx3 = sx4 = syx0 = syx1 = syx2 = 0.0;
	for (i = 0; i < N; ++i)
	{
	    x0 = 1.0;
	    x1 = x0*x[i];
	    x2 = x1*x[i];
	    x3 = x2*x[i];
	    x4 = x3*x[i];
	    yx0 = y[i];
	    yx1 = y[i]*x1;
	    yx2 = y[i]*x2;
	    sx0 += x0;
	    sx1 += x1;
	    sx2 += x2;
	    sx3 += x3;
	    sx4 += x4;
	    syx0 += yx0;
	    syx1 += yx1;
	    syx2 += yx2;
	}
	a[0] =               sx4;
	a[1] = b[0] =        sx3;
	a[2] = b[1] = c[0] = sx2;
	       b[2] = c[1] = sx1;
		      c[2] = sx0;
	d[0] = syx2;
	d[1] = syx1;
	d[2] = syx0;
	dinom = Det3d(a,b,c);
	*A = Det3d(d,b,c)/dinom;
	*B = Det3d(a,d,c)/dinom;
	*C = Det3d(a,b,d)/dinom;
}	/* end LeastSquareQuadr */

#define	Det2d(a,b) ((a)[0]*(b)[1] - (a)[1]*(b)[0]) 

extern void LeastSquareLinear(
	double *x,
	double *y,
	int N,
	double *A,
	double *B)
{
	double x0,x1,x2,yx0,yx1;
	double sx0,sx1,sx2,syx0,syx1;
	double a[2],b[2],c[2];
	double dinom;
	int i;

	x0 = x1 = x2 = yx0 = yx1 = 0.0;
	sx0 = sx1 = sx2 = syx0 = syx1 = 0.0;
	for (i = 0; i < N; ++i)
	{
	    x0 = 1.0;
	    x1 = x0*x[i];
	    x2 = x1*x[i];
	    yx0 = y[i];
	    yx1 = y[i]*x1;
	    sx0 += x0;
	    sx1 += x1;
	    sx2 += x2;
	    syx0 += yx0;
	    syx1 += yx1;
	}
	a[0] =        sx2;
	a[1] = b[0] = sx1;
	       b[1] = sx0;
	c[0] = syx1;
	c[1] = syx0;
	dinom = Det2d(a,b);
	*A = Det2d(c,b)/dinom;
	*B = Det2d(a,c)/dinom;
}	/* end LeastSquareLinear */

extern void ComputeNormalPrice(
	MARKET_DATA *data,
	int iseg,
	int order)
{
	int i,j,n,is,N;
	double a,b,c,base;
	double *x,*y;

	FT_VectorMemoryAlloc((POINTER*)&x,data->num_backtrace,sizeof(double));
	FT_VectorMemoryAlloc((POINTER*)&y,data->num_backtrace,sizeof(double));

	is = (iseg < data->num_backtrace + 1) ? 0 : 
			iseg - data->num_backtrace + 1;
	N = iseg - is + 1;
	if (N == 1 || order == 0)
	{
	    for (i = 0; i < data->num_assets; ++i)
	    {
		n = 0;
		for (j = is; j < is + N; ++j)
		{
		    x[n] = (double)j;
		    y[n] = data->assets[i].price[j];
		    n++;
		}
	    	LeastSquareConstant(x,y,N,&c);
		data->base[i] = base = c;
		data->assets[i].norm_price[iseg] = 
			data->assets[i].price[iseg]/base;
	    }
	}
	else if (N == 2 || order == 1)
	{
	    for (i = 0; i < data->num_assets; ++i)
	    {
		n = 0;
		for (j = is; j < is + N; ++j)
		{
		    x[n] = (double)j;
		    y[n] = data->assets[i].price[j];
		    n++;
		}
		LeastSquareLinear(x,y,N,&b,&c);
		data->base[i] = base = b*x[N-1] + c;
		data->assets[i].norm_price[iseg] = 
			data->assets[i].price[iseg]/base;
	    }
	}
	else
	{
	    for (i = 0; i < data->num_assets; ++i)
	    {
		n = 0;
		for (j = is; j < is + N; ++j)
		{
		    x[n] = (double)j;
		    y[n] = data->assets[i].price[j];
		    n++;
		}
		LeastSquareQuadr(x,y,N,&a,&b,&c);
		data->base[i] = base = a*sqr(x[N-1]) + b*x[N-1] + c;
		data->assets[i].norm_price[iseg] = 
			data->assets[i].price[iseg]/base;
	    }
	}
	if (iseg == data->num_segs-1)
	{
	    FILE *xfile;
	    char fname[100];
	    int k;
	    create_directory("base",NO);
	    for (i = 0; i < data->num_assets; ++i)
	    {
		n = 0;
		for (j = is; j < is + N; ++j)
		{
		    x[n] = (double)j;
		    y[n] = data->assets[i].price[j];
		    n++;
		}
		N = n;
		sprintf(fname,"base/base-%s",data->assets[i].asset_name);
		xfile = fopen(fname,"w");
	    	LeastSquareConstant(x,y,N,&c);
		fprintf(xfile,"Next\n");
        	fprintf(xfile,"color=red\n");
        	fprintf(xfile,"thickness = 1.5\n");
		for (k = 0; k < N; ++k)
		    fprintf(xfile,"%f  %f\n",x[k],y[k]);
		fprintf(xfile,"Next\n");
        	fprintf(xfile,"color=pink\n");
        	fprintf(xfile,"thickness = 1.5\n");
		for (k = 0; k < N; ++k)
		    fprintf(xfile,"%f  %f\n",x[k],c);
		LeastSquareQuadr(x,y,N,&a,&b,&c);
		fprintf(xfile,"Next\n");
        	fprintf(xfile,"color=green\n");
        	fprintf(xfile,"thickness = 1.5\n");
		for (k = 0; k < N; ++k)
		    fprintf(xfile,"%f  %f\n",x[k],a*sqr(x[k]) + b*x[k] + c);
		LeastSquareLinear(x,y,N,&b,&c);
		fprintf(xfile,"Next\n");
        	fprintf(xfile,"color=blue\n");
        	fprintf(xfile,"thickness = 1.5\n");
		for (k = 0; k < N; ++k)
			fprintf(xfile,"%f  %f\n",x[k],b*x[k] + c);
		fprintf(xfile,"Next\n");
        	fprintf(xfile,"color=aqua\n");
        	fprintf(xfile,"thickness = 1.5\n");
		n = 0;
		for (j = is; j < is + N; ++j)
		{
		    y[n] = data->assets[i].price[j]/
				data->assets[i].norm_price[j];
		    n++;
		}
		for (k = 0; k < N; ++k)
		{
		    fprintf(xfile,"%f  %f\n",x[k],y[k]);
		}
		fclose(xfile);
	    }
	}
	FT_FreeThese(2,x,y);
}	/* end ComputeNormalPrice */

extern void ModifyMarketData(
	MARKET_DATA *data)
{
	int i,j,m,n;
	int istar;		/* center asset index */
	char string[100];
	double ave_value;
	double mfactor;

	printf("Type yes to change data set: ");
	scanf("%s",string);
	if (string[0] != 'y' && string[0] != 'Y')
	    return;

	printf("Type yes to add assets: ");
	scanf("%s",string);
	if (string[0] == 'y' || string[0] == 'Y')
	{
	    data->new_data = YES;
	}

	printf("Type yes to delete assets: ");
	scanf("%s",string);
	if (string[0] == 'y' || string[0] == 'Y')
	{
	    DeleteAssets(data);
	}

	printf("Type yes to delete old data: ");
	scanf("%s",string);
	if (string[0] == 'y' || string[0] == 'Y')
	{
	    DeleteOldData(data);
	    data->new_data = YES;
	}

	printf("Type yes to change number of back trace: ");
	scanf("%s",string);
	if (string[0] == 'y' || string[0] == 'Y')
	{
	    printf("Current back trace: %d\n",data->num_backtrace);
	    printf("Enter number of back trace: ");
	    scanf("%d",&data->num_backtrace);
	    data->new_data = YES;
	}

	printf("Type yes to modify data base: ");
	scanf("%s",string);
	if (string[0] != 'y' && string[0] != 'Y')
	    return;
	ComputeAllNormPrice(data,0);
}	/* end ModifyMarketData */

extern void ModifyAccount(
	PORTFOLIO *data)
{
	char string[100];
	printf("Type yes to change account profile slope: ");
	scanf("%s",string);
	if (string[0] == 'y' || string[0] == 'Y')
	{
	    printf("Current profile slope: %f\n",data->polar_ratio);
	    printf("Enter account profile slope: ");
	    scanf("%lf",&data->polar_ratio);
	}
   	PromptForDataMap(data);

}	/* end ModifyAccount */

static void AddAssets(
	MARKET_DATA *data)
{
	int i,j,num_add;
	ASSET *new_assets;
	int new_num_assets;
	boolean *new_data_map;
	char **names;
	double *new_base;

	printf("Current number of assets: %d\n",data->num_assets);
	printf("Enter number of assets to be added: ");
	scanf("%d",&num_add);
	if (num_add > 10)
	{
	    printf("Cannot add more than 10 assets at once!\n");
	    return;
	}

	new_num_assets = data->num_assets + num_add;
	FT_VectorMemoryAlloc((POINTER*)&new_assets,new_num_assets+1,
                                sizeof(ASSET));
	FT_VectorMemoryAlloc((POINTER*)&new_base,new_num_assets+1,
                                sizeof(double));
	FT_VectorMemoryAlloc((POINTER*)&new_data_map,new_num_assets+1,
                                sizeof(boolean));
	FT_MatrixMemoryAlloc((POINTER*)&names,new_num_assets+1,100,
                                sizeof(char));
	for (j = 0; j < new_num_assets+1; ++j)
        {
	    FT_VectorMemoryAlloc((POINTER*)&new_assets[j].price,
                                data->num_segs+10,sizeof(double));
            FT_VectorMemoryAlloc((POINTER*)&new_assets[j].norm_price,
                                data->num_segs+10,sizeof(double));
	    if (j < data->num_assets)
	    {
		new_base[j] = data->base[j];
		strcpy(new_assets[j].asset_name,data->assets[j].asset_name);
		strcpy(new_assets[j].color,data->assets[j].color);
		for (i = 0; i < data->num_segs; ++i)
		{
		    new_assets[j].price[i] = data->assets[j].price[i];
		    new_assets[j].norm_price[i] = data->assets[j].norm_price[i];
		}
	    }
	    else if (j < new_num_assets)
	    {
		printf("Enter name of the new asset: ");
		scanf("%s",new_assets[j].asset_name);
		strcpy(new_assets[j].color,Xcolor[j%12]);
		strcpy(names[j],new_assets[j].asset_name);
		GetCurrentPrice(names+j,&new_base[j],1);
		printf("%s: color %s  base = %f\n",
				new_assets[j].asset_name,
				new_assets[j].color,
				new_base[j]);
		for (i = 0; i < data->num_segs; ++i)
		{
		    new_assets[j].price[i] = new_base[j];
		    new_assets[j].norm_price[i] = 1.0;
		}
		new_data_map[j] = YES;
	    }
	    else
	    {
		int im = data->num_assets;
		new_base[j] = data->base[im];
		strcpy(new_assets[j].asset_name,data->assets[im].asset_name);
		strcpy(new_assets[j].color,data->assets[im].color);
		for (i = 0; i < data->num_segs; ++i)
		{
		    new_assets[j].price[i] = data->assets[im].price[i];
		    new_assets[j].norm_price[i] = 
				data->assets[im].norm_price[i];
		}
	    }
	}
	for (j = 0; j < data->num_assets+1; ++j)
	    FT_FreeThese(2,data->assets[j].price,data->assets[j].norm_price);
	FT_FreeThese(1,data->assets);
	
	data->assets = new_assets;
	data->new_data = YES;
	data->num_assets = new_num_assets;
}	/* end AddAssets */

static void DeleteAssets(
        MARKET_DATA *data)
{
}       /* end DeleteAssets */

extern void PromptForDataMap(
	PORTFOLIO *account)
{
	int i;
	char string[100];
	MARKET_DATA *data = account->data;

	printf("Type yes to change data map: ");
	scanf("%s",string);
	if (string[0] != 'y' && string[0] != 'Y') return;

	for (i = 0; i < data->num_assets; ++i)
	{
	    printf("Include asset %s ? ",data->assets[i].asset_name);
	    scanf("%s",string);
	    if (string[0] == 'y' || string[0] == 'Y')
	    	account->data_map[i] = YES;
	    else
	    	account->data_map[i] = NO;
	}
}	/* end PromptForDataMap */

extern void AddData(
	MARKET_DATA *data,
	boolean silence)
{
	int j,m,M;
	char string[100];
	double ave_value;
	char **stock_names;
	double *current_price;

	m = data->num_segs;
	M = data->num_assets;

	FT_VectorMemoryAlloc((POINTER*)&stock_names,M,sizeof(char*));
	FT_VectorMemoryAlloc((POINTER*)&current_price,M,sizeof(double));
	for (j = 0; j < M; ++j)
	    stock_names[j] = data->assets[j].asset_name;
	GetCurrentPrice(stock_names,current_price,M);
	if (!silence)
	{
	    printf("Current prices:\n");
	    for (j = 0; j < M; ++j)
	    	printf("\t%5s: %f\n",stock_names[j],current_price[j]);
	    printf("Type yes to add to database: ");
	    scanf("%s",string);
	    if (string[0] != 'y' && string[0] != 'Y')
	    {
	    	FT_FreeThese(2,stock_names,current_price);
	    	return;
	    }
	}

	for (j = 0; j < M; ++j)
	{
	    data->assets[j].price[m] = current_price[j];
	    if (data->assets[j].price[m] == -1)
		data->assets[j].price[m] = data->assets[j].price[m-1];
	}
	data->num_segs += 1;
	ComputeAllNormPrice(data,0);

	m = data->num_segs - 1;
	ave_value = 0.0;
	for (j = 0; j < M; ++j)
	    ave_value += data->assets[j].norm_price[m];
	data->base[j] = 1.0;
	data->assets[j].price[m] = ave_value/data->num_assets;
	data->assets[j].norm_price[m] = data->assets[j].price[m];
	data->new_data = YES;
	FT_FreeThese(2,stock_names,current_price);
}	/* end AddData */

extern void XgraphData(
	MARKET_DATA *ori_data,
	boolean *account_data_map)
{
	char fname[100];
	FILE *xfile;
	int i,j,k,M,is;
	double time;
	double *value,ave_value;
	MARKET_DATA *data = CopyMarketData(ori_data);
	boolean *data_map = account_data_map;

	
	M = data->num_assets;
	if (data_map == NULL)
	{
	    char string[10];
	    FT_VectorMemoryAlloc((POINTER*)&data_map,M,sizeof(boolean));
	    for (i = 0; i < M; ++i)
	    {
		data_map[i] = NO;
		printf("Include %4s: ",data->assets[i].asset_name);
		scanf("%s",string);
		if (string[0] == 'y')
		    data_map[i] = YES;
	    }
	}
	AddData(data,YES);
	create_directory("xg",NO);
	is = (data->num_segs < data->num_backtrace) ? 0 :
		data->num_segs - data->num_backtrace;
	for (j = 0; j < M; ++j)
	{
	    if (data_map[j] != YES)
	    	continue;
	    sprintf(fname,"xg/N-%d-.xg",j);
	    xfile = fopen(fname,"w");
	    fprintf(xfile,"color=%s\n",data->assets[j].color);
	    fprintf(xfile,"thickness = 1.5\n");
	    value = data->assets[j].norm_price;
	    for (i = is; i < data->num_segs; ++i)
	    {
		if (value[i] == 0.0) continue;
		time = (double)i;
		fprintf(xfile,"%f %f\n",time,value[i]);
	    }
	    fclose(xfile);
	}
	for (j = 0; j < M-1; ++j)
	for (k = j+1; k < M; ++k)
	{
	    if (data_map[j] != YES || data_map[k] != YES)
	    	continue;
	    sprintf(fname,"xg/M-%d-%d.xg",j,k);
	    xfile = fopen(fname,"w");
	    fprintf(xfile,"color=green\n");
	    fprintf(xfile,"thickness = 1.5\n");
	    for (i = is; i < data->num_segs; ++i)
	    {
		if (value[i] == 0.0) continue;
		time = (double)i;
		ave_value = 0.5*(data->assets[j].norm_price[i] +
				 data->assets[k].norm_price[i]);
		fprintf(xfile,"%f %f\n",time,ave_value);
	    }
	    fclose(xfile);
	}
	for (j = 0; j < M-1; ++j)
	for (k = j+1; k < M; ++k)
	{
	    if (data_map[j] != YES || data_map[k] != YES)
	    	continue;
	    sprintf(fname,"xg/D-%d-%d.xg",j,k);
	    xfile = fopen(fname,"w");

	    fprintf(xfile,"color=%s\n",data->assets[j].color);
	    fprintf(xfile,"thickness = 1.5\n");
	    for (i = is; i < data->num_segs; ++i)
	    {
		if (value[i] == 0.0) continue;
		time = (double)i;
		ave_value = 0.5*(data->assets[j].norm_price[i] +
				 data->assets[k].norm_price[i]);
		fprintf(xfile,"%f %f\n",time,
			100*(data->assets[j].norm_price[i] - ave_value));
	    }
	    fprintf(xfile,"Next\n");
	    fprintf(xfile,"color=%s\n",data->assets[k].color);
	    fprintf(xfile,"thickness = 1.5\n");
	    for (i = is; i < data->num_segs; ++i)
	    {
		if (value[i] == 0.0) continue;
		time = (double)i;
		ave_value = 0.5*(data->assets[j].norm_price[i] +
				 data->assets[k].norm_price[i]);
		fprintf(xfile,"%f %f\n",time,
			100*(data->assets[k].norm_price[i] - ave_value));
	    }
	    fclose(xfile);
	}
   	DataTrend(data,data_map);
	FreeMarketData(data);
}	/* end XgraphData */

extern void TradeInfo(
	PORTFOLIO *account)
{
	int i,j;
	char string[100];
	boolean closed_trade_exist = NO;
	boolean partial_loop_exist = NO;

	SortTradeOrder(account);
	PrintOpenTrade(account);
	for (i = 0; i < account->num_trades; ++i)
	{
	    if (account->trades[i].status != OPEN)
	    {
	    	PrintClosedTradeLoop(NULL,account->trades[i],account);
		if (account->trades[i].status == CLOSED)
		    closed_trade_exist = YES;
		else if (account->trades[i].status == PARTIAL_CLOSED)
		    partial_loop_exist = YES;
	    }
	}
	if (closed_trade_exist)
	{
	    printf("Type yes to save and delete closed tradings: ");
	    scanf("%s",string);
	    if (string[0] == 'y' || string[0] == 'Y')
	    {
		SaveDeleteClosedTradeLoop(account);
		WriteAccountData(account);
	    }
	}
	if (partial_loop_exist)
	{
	    printf("Choose to save-close (s) or wrap (w) the loop: ");
	    scanf("%s",string);
	    if (string[0] == 's' || string[0] == 'S')
	    {
		SaveDeleteClosedTradeLoop(account);
		WriteAccountData(account);
	    }
	    else if (string[0] == 'w' || string[0] == 'W')
	    {
		WrapPartialTradeLoop(account);
		WriteAccountData(account);
	    }
	}
	printf("Type yes to print linear profile: ");
	scanf("%s",string);
	if (string[0] == 'y' || string[0] == 'Y')
	    PrintCurrentLinearProfile(account);
}	/* end TradeInfo */

extern void DataCompare(
	MARKET_DATA *data)
{
	int i1,i2,j;
	double nu1,nu2;
	double mu1,mu2;
	double norm_price1,norm_price2,dvalue;
	char string[100];
	int is,ie;
	char *stock_names[2];
	double prices[2];

	PrintAssetList(data);
	printf("Enter indices of two assets: ");
	scanf("%d %d",&i1,&i2);
	stock_names[0] = data->assets[i1].asset_name;
	stock_names[1] = data->assets[i2].asset_name;
	GetCurrentPrice(stock_names,prices,2);
	norm_price1 = 100.0*prices[0]/data->base[i1];
	norm_price2 = 100.0*prices[1]/data->base[i2];
	dvalue = norm_price1 - norm_price2;
	if (dvalue > 0.0)
	{
	    printf("\n%4s > %4s  %5.2f percent\n\n",stock_names[0],
				stock_names[1],dvalue);
	}
	else
	{
	    printf("\n%4s < %4s  %5.2f percent\n\n",stock_names[0],
				stock_names[1],-dvalue);
	}

	printf("Type yes to calculate amplification factor: ");
	scanf("%s",string);
	if (string[0] == 'y' || string[0] == 'Y')
	{
	    printf("Enter old values of two assets: ");
	    scanf("%lf %lf",&nu1,&nu2);
	    printf("Enter new values of two assets: ");
	    scanf("%lf %lf",&mu1,&mu2);
	    printf("The amplification factor = %f\n",nu1*mu2/nu2/mu1);
	}
}	/* end DataCompare */

extern void DataTrend(
	MARKET_DATA *data,
	boolean *data_map)
{
	int i,j,k,is,N;
	int M = data->num_assets;
	double QA,QB,QC,LA,LB,mLA,mLB;
	FILE *sfile1,*sfile2;
	FILE *vfile0,*vfile1;
	FILE *mfile0,*mfile1;
	double *value,time[MAX_TRACE],mean[MAX_TRACE];
	double value_ave,mean_ave[MAX_TRACE];
	double *pmean,*pmean_ave;
	double s0_max,s0[20],s1[20];
	int is0[20],is1[20];
	int is0_max;
	int iswap;
	char string[100];
	int num_data = 0;

	vfile0 = fopen("xg/v-0.xg","w");
	vfile1 = fopen("xg/v-1.xg","w");
	mfile0 = fopen("xg/m-0.xg","w");
	mfile1 = fopen("xg/m-1.xg","w");
	sfile1 = fopen("xg/s-1.xg","w");
	sfile2 = fopen("xg/s-2.xg","w");

	N = data->num_backtrace - 12;
	if (N > MAX_TRACE - 12) N = MAX_TRACE - 12;

	for (i = 0; i < N; ++i) 
	    time[i] = i*0.25;
	is = data->num_segs - N - 12;
	for (i = 0; i < N + 12; ++i) 
	{
	    mean[i] = 0.0;
	    mean_ave[i] = 0.0;
	    num_data = 0;
	    for (j = 0; j < M; ++j)
	    {
		if (!data_map[j]) continue;
		value = data->assets[j].norm_price + is + i;
		mean[i] += value[0];
		value_ave = (0.5*(value[0] + value[-3]) +
				  value[-1] + value[-2])/3.0;
		mean_ave[i] += value_ave;
		num_data++;
	    }
	    mean[i] /= num_data;
	    mean_ave[i] /= num_data;
	}

	for (i = 0; i < M; ++i)
	{
	    if (data_map[i] != YES)
	    	continue;
	    fprintf(sfile1,"color=%s\n",data->assets[i].color);
	    fprintf(sfile1,"thickness = 1.5\n");
	    fprintf(sfile2,"color=%s\n",data->assets[i].color);
	    fprintf(sfile2,"thickness = 1.5\n");
	    fprintf(vfile0,"color=%s\n",data->assets[i].color);
	    fprintf(vfile0,"thickness = 1.5\n");
	    fprintf(vfile1,"color=%s\n",data->assets[i].color);
	    fprintf(vfile1,"thickness = 1.5\n");
	    fprintf(mfile0,"color=%s\n",data->assets[i].color);
	    fprintf(mfile0,"thickness = 1.5\n");
	    fprintf(mfile1,"color=%s\n",data->assets[i].color);
	    fprintf(mfile1,"thickness = 1.5\n");

	    for (j = 0; j < N; ++j)
	    {
		is = data->num_segs - N - 3;
		value = data->assets[i].norm_price + is + j;
		pmean = mean + j + 9;
		pmean_ave = mean_ave + j + 9;
		/* Value of the quater day */
		fprintf(vfile0,"%f  %f\n",time[j],100.0*value[3]);

		/* Average value of the day */
		value_ave = (0.5*(value[0] + value[3]) +
				value[1] + value[2])/3.0;
		fprintf(vfile1,"%f  %f\n",time[j],100.0*value_ave);
		if (j == N-1) s0[i] = 100.0*(value[3] - value[2])/0.25;

		fprintf(mfile0,"%f  %f\n",time[j],100.0*(value[3] - pmean[3]));
		fprintf(mfile1,"%f  %f\n",time[j],
				100.0*(value_ave - pmean_ave[3]));

		/* Average slope of the day */
		LeastSquareLinear(time,value,4,&LA,&LB);
		fprintf(sfile1,"%f  %f\n",time[j],100.0*LA);
		/* Average concavity of the day */

		/* Average slope of two days */
		is = data->num_segs - N - 7;
		value = data->assets[i].norm_price + is + j;

		LeastSquareLinear(time,value,8,&LA,&LB);
		fprintf(sfile2,"%f  %f\n",time[j],100.0*LA);
		/* Average concavity of two days */

	    }
	    fprintf(vfile0,"Next\n");
	    fprintf(vfile1,"Next\n");
	    fprintf(sfile1,"Next\n");
	    fprintf(sfile2,"Next\n");
	    fprintf(mfile0,"Next\n");
	    fprintf(mfile1,"Next\n");
	}
	fprintf(vfile0,"Next\n");
	fprintf(vfile0,"color=aqua\n");
	fprintf(vfile0,"thickness = 2\n");
	for (j = 0; j < N; ++j)
	{
	    is = data->num_segs - N - 3;
	    pmean = mean + j + 9;
	    fprintf(vfile0,"%f  %f\n",time[j],100.0*pmean[3]);
	}
	fclose(vfile0);
	fclose(vfile1);
	fclose(sfile1);
	fclose(sfile2);
	fclose(mfile0);
	fclose(mfile1);
}	/* end DataTrend */

static void PrintRateOfChange(
	MARKET_DATA *data,
	boolean *data_map)
{
	int i,n,M;
	double base,fit_value;
	double Q[3],L[2];

	n = data->num_segs-1;
	M = data->num_assets;

	printf("\n");
	printf("          Immediate   "
	       		"     One-day     "
	       		"          Two-Day\n");
	printf("               L    "
	       		"   L      Q      M"
	       		"       L      Q      M\n");
	for (i = 0; i < M; ++i)
	{
	    if (data_map[i] != YES)
	        continue;
	    base = data->base[i];
	    printf("%2d  %5s: ",i,data->assets[i].asset_name);
	    L[0] = (data->assets[i].norm_price[n] - 
			data->assets[i].norm_price[n-1])/0.25;
	    printf(" %6.2f ",100*L[0]);
	    GetLeastSquare(data,i,5,Q,L);
	    fit_value = base*(Q[0]+Q[1]+Q[2]);
	    printf("|%6.2f ",100*L[0]);
	    printf("%6.2f ",100*Q[0]);
	    printf("%6.2f ",fit_value);
	    GetLeastSquare(data,i,9,Q,L);
	    fit_value = base*(Q[0]*sqr(2.0)+Q[1]*2.0+Q[2]);
	    printf("|%6.2f ",100*L[0]);
	    printf("%6.2f ",100*Q[0]);
	    printf("%6.2f ",fit_value);
	    printf("\n");
	}
}	/* end PrintRateOfChange */

extern void ReadMarketData(
	MARKET_DATA *data)
{
	char *data_name = data->data_name;
	FILE *infile = fopen(data_name,"r");
	int i,j;
	int order;
	
	if (infile == NULL)
	{
	    double ave_value = 0.0;
	    double ave_base = 0.0;
	    printf("Enter data set name: ");
	    scanf("%s",data->data_name);
	    sprintf(data_name,"%s",data->data_name);
	    printf("Enter number of assets: ");
	    scanf("%d",&data->num_assets);
	    data->num_segs = 1;
	    FT_VectorMemoryAlloc((POINTER*)&data->assets,data->num_assets+1,
				sizeof(ASSET));
	    FT_VectorMemoryAlloc((POINTER*)&data->base,data->num_assets+1,
				sizeof(double));

	    for (j = 0; j < data->num_assets+1; ++j)
	    {
		FT_VectorMemoryAlloc((POINTER*)&data->assets[j].price,
				data->num_segs+10,sizeof(double));
		FT_VectorMemoryAlloc((POINTER*)&data->assets[j].norm_price,
				data->num_segs+10,sizeof(double));
		if (j < data->num_assets)
		{
		    printf("Enter name of asset %d: ",j+1);
		    scanf("%s",data->assets[j].asset_name);
		    printf("Enter color of asset %d: ",j+1);
		    scanf("%s",data->assets[j].color);
		    printf("Enter value of asset %d: ",j+1);
		    scanf("%lf",&data->assets[j].price[0]);
		    ave_value += data->assets[j].price[0];
		    printf("Enter base value of asset %d: ",j+1);
		    scanf("%lf",&data->base[j]);
		    data->assets[j].norm_price[0] = data->assets[j].price[0]/
				data->base[j];
		    ave_base += data->base[j];
		}
		else
		{
		    data->assets[j].price[0] = 1.0;
		    data->base[j] = 1.0;
		    data->assets[j].norm_price[0] = 1.0;
		    sprintf(data->assets[j].asset_name,"Mean");
		    sprintf(data->assets[j].color,"green");
		}
	    }
	}
	else
	{
	    double ave_value = 0.0;
	    fscanf(infile,"%*s");
	    fscanf(infile,"%d",&data->num_assets);
	    fscanf(infile,"%d",&data->num_segs);
	    fscanf(infile,"%d",&data->num_backtrace);
	    FT_VectorMemoryAlloc((POINTER*)&data->assets,data->num_assets+1,
				sizeof(ASSET));
	    FT_VectorMemoryAlloc((POINTER*)&data->base,data->num_assets+1,
				sizeof(double));
	    for (j = 0; j < data->num_assets+1; ++j)
	    {
		fscanf(infile,"%s\n",data->assets[j].asset_name);
		fscanf(infile,"%s\n",data->assets[j].color);
		fscanf(infile,"%lf\n",&data->base[j]);
		FT_VectorMemoryAlloc((POINTER*)&data->assets[j].price,
				data->num_segs+10,sizeof(double));
		FT_VectorMemoryAlloc((POINTER*)&data->assets[j].norm_price,
				data->num_segs+10,sizeof(double));
	    }
	    for (i = 0; i < data->num_segs; ++i)
	    {
	    	for (j = 0; j < data->num_assets+1; ++j)
		    fscanf(infile,"%lf ",&data->assets[j].price[i]);
	    }
	    fclose(infile);
	}
	ComputeAllNormPrice(data,0);
	for (i = 0; i < data->num_segs; ++i)
	{
	    double ave_value = 0.0;
	    for (j = 0; j < data->num_assets; ++j)
		ave_value += data->assets[j].norm_price[i];
	    data->assets[j].price[i] = ave_value/data->num_assets;
	    data->assets[j].norm_price[i] = ave_value/data->num_assets;
	}
	data->new_data = NO;
}	/* end ReadMarketData */

extern void ReadAccountData(
	PORTFOLIO *account)
{
	char *account_name = account->account_name;
	FILE *infile = fopen(account_name,"r");
	MARKET_DATA *data = account->data;
	int i,j;
	
        FT_VectorMemoryAlloc((POINTER*)&account->shares,data->num_assets+10,
				sizeof(int));
	FT_VectorMemoryAlloc((POINTER*)&account->data_map,data->num_assets+11,
				sizeof(boolean));
        for (j = 0; j < data->num_assets; ++j)
	    account->shares[j] = 0;
	if (infile == NULL)
	{
            for (j = 0; j < data->num_assets; ++j)
		account->data_map[j] = YES;
	    return;
	}
	else
	{
	    if (fgetstring(infile,"Account Investment Polarization Ratio ="))
	    {
		fscanf(infile,"%lf",&account->polar_ratio);
	    }
	    if (fgetstring(infile,"Data Map"))
	    {
                for (j = 0; j < data->num_assets; ++j)
                    fscanf(infile,"%d",(int*)account->data_map+j);
	    }
	    if (fgetstring(infile,"Investment Portfolio"))
            {
                for (j = 0; j < data->num_assets; ++j)
                    fscanf(infile,"%d",account->shares+j);
            }
            FT_VectorMemoryAlloc((POINTER*)&account->trades,MAX_NUM_TRADE,
                                        sizeof(TRADE));
            if (fgetstring(infile,"Trading Record"))
            {
                fscanf(infile,"%d",&account->num_trades);
                for (j = 0; j < account->num_trades; ++j)
		{
                    fscanf(infile,"%d",&account->trades[j].num_stages);
                    fscanf(infile,"%d",(int*)&account->trades[j].status);
		    for (i = 0; i < account->trades[j].num_stages; ++i)
		    {
                    	fscanf(infile,"%d %d\n",
				&account->trades[j].index_sell[i],
                                &account->trades[j].index_buy[i]);
                    	fscanf(infile,"%d %d\n",
				&account->trades[j].num_shares_sell[i],
                                &account->trades[j].num_shares_buy[i]);
                    	fscanf(infile,"%lf %lf\n",
				&account->trades[j].price_sell[i],
                                &account->trades[j].price_buy[i]);
		    }
		}
            }
            if (fgetstring(infile,"Index of Share Equivalence"))
            {
                fscanf(infile,"%d\n",&account->eindex);
	    }
	    fclose(infile);
	}
}	/* end ReadAccountData */

extern void WriteMarketData(
	MARKET_DATA *data)
{
	int i,j,m;
	char string[100];
	char *data_name = data->data_name;
	char dirname[256],fname[256];
	FILE *bfile,*outfile;

	if (!data->new_data) return;
	ComputeAllNormPrice(data,0);
	sprintf(dirname,"base/%s",data->data_name);
	create_directory(dirname,NO);
    	for (m = 0; m < data->num_assets; ++m)	
    	{
	    sprintf(fname,"%s/%s",dirname,data->assets[m].asset_name);
	    if ((bfile = fopen(fname,"r")) == NULL)
		bfile = fopen(fname,"w");
	    else
	    {
		fclose(bfile);
		bfile = fopen(fname,"a");
	    }
	    fprintf(bfile,"%f  %f\n",(double)data->num_segs,
				data->base[m]);
	    fclose(bfile);
	}
	outfile = fopen(data_name,"w");
	fprintf(outfile,"%s\n",data->data_name);
	fprintf(outfile,"%d\n",data->num_assets);
	fprintf(outfile,"%d\n",data->num_segs);
	fprintf(outfile,"%d\n",data->num_backtrace);
	for (j = 0; j < data->num_assets+1; ++j)
	{
	    fprintf(outfile,"%s\n",data->assets[j].asset_name);
	    fprintf(outfile,"%s\n",data->assets[j].color);
	    fprintf(outfile,"%f\n",data->base[j]);
	}
	for (i = 0; i < data->num_segs; ++i)
	{
	    for (j = 0; j < data->num_assets+1; ++j)
	    {
		fprintf(outfile,"%f ",data->assets[j].price[i]);
	    }
	    fprintf(outfile,"\n");
	}
	fclose(outfile);
	data->new_data = NO;
}	/* end WriteMarketData */

extern void WriteAccountData(
	PORTFOLIO *account)
{
	FILE *outfile;	
	int i,j;
	char string[100];
	char *account_name = account->account_name;
	MARKET_DATA *data = account->data;

	outfile = fopen(account_name,"w");
	fprintf(outfile,"%s\n",account->account_name);
	fprintf(outfile,"Account Investment Polarization Ratio = %f\n",
				account->polar_ratio);
	fprintf(outfile,"Data Map\n");
	for (i = 0; i < data->num_assets; ++i)
	    fprintf(outfile,"%d\n",account->data_map[i]);
	if (account->shares)
	{
	    fprintf(outfile,"Investment Portfolio\n");
	    for (j = 0; j < data->num_assets; ++j)
		fprintf(outfile,"%d\n",account->shares[j]);
	}
	if (account->num_trades != 0)
	{
	    SortTradeOrder(account);
	    fprintf(outfile,"Trading Record\n");
	    fprintf(outfile,"%d\n",account->num_trades);
	    for (j = 0; j < account->num_trades; ++j)
	    {
	    	fprintf(outfile,"%d\n",account->trades[j].num_stages);
	    	fprintf(outfile,"%d\n",account->trades[j].status);
		for (i = 0; i < account->trades[j].num_stages; ++i)
		{
	    	    fprintf(outfile,"%d %d\n",account->trades[j].index_sell[i],
				account->trades[j].index_buy[i]);
	    	    fprintf(outfile,"%d %d\n",
				account->trades[j].num_shares_sell[i],
				account->trades[j].num_shares_buy[i]);
	    	    fprintf(outfile,"%f %f\n",account->trades[j].price_sell[i],
				account->trades[j].price_buy[i]);
		}
	    }
	}
        fprintf(outfile,"Index of Share Equivalence\n");
        fprintf(outfile,"%d\n",account->eindex);
	fclose(outfile);
}	/* end WriteAccountData */

extern void RankData(
	MARKET_DATA *data,
	boolean *account_data_map)
{
	int i,j;
	int n = data->num_segs-1;
	int M = data->num_assets;
	double dvalue,d2value;
	char string[100];
	double Q[3],L[2];
	double fit_value,base;
	int *isort;
	double mean_norm_price = 0.0;
	int num_ave = 0;
	int is_first,is_last;
	MARKET_DATA *copy_data;
	boolean *data_map = account_data_map;
	int order;

	copy_data = CopyMarketData(data);
	printf("Number of backtrace: %d\n",copy_data->num_backtrace);

	if (data_map == NULL)
	{
	    char string[10];
	    FT_VectorMemoryAlloc((POINTER*)&data_map,M,sizeof(boolean));
	    for (i = 0; i < M; ++i)
	    {
		data_map[i] = NO;
		printf("Include %4s: ",data->assets[i].asset_name);
		scanf("%s",string);
		if (string[0] == 'y')
		    data_map[i] = YES;
	    }
	}

start_ranking:
	printf("Enter new number of backtrace (current %d): ",
				copy_data->num_backtrace);
	scanf("%d",&copy_data->num_backtrace);
	printf("Enter least square order: ");
	scanf("%d",&order);
	ComputeAllNormPrice(copy_data,order);

	FT_VectorMemoryAlloc((POINTER*)&isort,M,sizeof(int));
	for (i = 0; i < M; ++i) 
	{
	    isort[i] = i;
	    num_ave++;
	    mean_norm_price += copy_data->assets[i].norm_price[n];
	}

	for (i = 0; i < M-1; ++i)
	for (j = i+1; j < M; ++j)
	{
	    if (copy_data->assets[isort[i]].norm_price[n] < 
		copy_data->assets[isort[j]].norm_price[n])
	    {
		int itmp = isort[j];
		isort[j] = isort[i];
		isort[i] = itmp;
	    }
	}
	is_first = is_last = -1;
	for (i = 0; i < M; ++i)
	{
	    if (data_map[isort[i]] == YES && is_first == -1)
		is_first = i;
	    if (data_map[isort[M-i-1]] == YES && is_last == -1)
		is_last = M-i-1;
	}
	printf("\t");
	for (i = 0; i < M; ++i)
	{
	    if (i == is_first) continue;
	    if (data_map[isort[i]] != YES)
	    	continue;
	    printf("%6s  ",copy_data->assets[isort[i]].asset_name);
	}
	printf("\n");
	for (i = 0; i < M; ++i)
	{
	    if (data_map[isort[i]] != YES)
	    	continue;
	    if (i == is_last) 
	    {
	    	printf("\n");
		break;
	    }
	    printf("%6s  ",copy_data->assets[isort[i]].asset_name);
	    for (j = 0; j <= i; ++j)
	    {
	    	if (j == is_first) continue;
	    	if (data_map[isort[j]] != YES)
	    	    continue;
		printf("        ");
	    }
	    for (j = i+1; j < M; ++j)
	    {
	    	if (data_map[isort[j]] != YES)
	    	    continue;
		double dvalue = (copy_data->assets[isort[i]].norm_price[n]
			- copy_data->assets[isort[j]].norm_price[n])*100.0;
		printf("%6.2f  ",dvalue);
	    }
	    printf("\n");
	}
	printf("Type yes to re-rank: ");
	scanf("%s",string);
	if (string[0] == 'y')
	{
	    goto start_ranking;
	}
	FT_FreeThese(1,isort);
	FreeMarketData(copy_data);
}	/* end RankData */


extern void PrintAssetList(
	MARKET_DATA *data)
{
	int i;
	for (i = 0; i < data->num_assets; ++i)
	{
	    printf("Assets %d: %4s\n",i,data->assets[i].asset_name);
	}
}	/* end PrintAssetList */

extern void InitTrade(
	MARKET_DATA *data)
{
	int i1,i2,j;
	double nu1,nu2;
	double mu1,mu2;
	double norm_price1,norm_price2,dvalue;
	char string[100];
	int is,ie;
	char fname[200];
	int i;
	FILE *xfile;
	double time,ratio,v[2],v_ave;
	char *stock_names[2];

	PrintAssetList(data);
	printf("Enter indices of two assets: ");
	scanf("%d %d",&i1,&i2);
	is = (data->num_segs < data->num_backtrace) ? 0 :
                	data->num_segs - data->num_backtrace;
	ie = data->num_segs;
	stock_names[0] = data->assets[i1].asset_name;
	stock_names[1] = data->assets[i2].asset_name;
	GetCurrentPrice(stock_names,v,2);
	create_directory("xg",NO);
	sprintf(fname,"xg/I-ratio.xg");
	xfile = fopen(fname,"w");
        fprintf(xfile,"color=red\n");
        fprintf(xfile,"thickness = 1.5\n");
	for (i = is; i < ie; ++i)
        {
            time = (double)i;
	    ratio = data->assets[i1].norm_price[i]/
				data->assets[i2].norm_price[i];
            fprintf(xfile,"%f %f\n",time,ratio);
        }
        fprintf(xfile,"%f %f\n",time+1.0,v[0]/data->base[i1]
				/v[1]*data->base[i2]);
        fprintf(xfile,"\nNext\n");
        fprintf(xfile,"color=green\n");
        fprintf(xfile,"thickness = 1.5\n");
	for (i = is; i < ie; ++i)
        {
            time = (double)(i-2);
	    ratio = 0.0;
	    for (j = i-3; j <= i; ++j)
	    	ratio += data->assets[i1].norm_price[j]/
				data->assets[i2].norm_price[j];
	    ratio /= 4.0;
            fprintf(xfile,"%f %f\n",time,ratio);
        }
	if (string[0] == 'y' || string[0] == 'Y')
	{
            time = (double)(i-2);
	    ratio = 0.0;
	    for (j = i-3; j < i; ++j)
	    	ratio += data->assets[i1].norm_price[j]/
				data->assets[i2].norm_price[j];
	    ratio += v[0]/data->base[i1]
                                /v[1]*data->base[i2];
	    ratio /= 4.0;
            fprintf(xfile,"%f %f\n",time,ratio);
	}
        fprintf(xfile,"\nNext\n");
        fprintf(xfile,"color=blue\n");
        fprintf(xfile,"thickness = 1.5\n");
	for (i = is; i < ie; ++i)
        {
            time = (double)(i-6);
	    ratio = 0.0;
	    for (j = i-11; j <= i; ++j)
	    	ratio += data->assets[i1].norm_price[j]/
				data->assets[i2].norm_price[j];
	    ratio /= 12.0;
            fprintf(xfile,"%f %f\n",time,ratio);
        }
	if (string[0] == 'y' || string[0] == 'Y')
	{
            time = (double)(i-6);
	    ratio = 0.0;
	    for (j = i-11; j < i; ++j)
	    	ratio += data->assets[i1].norm_price[j]/
				data->assets[i2].norm_price[j];
	    ratio += v[0]/data->base[i1]
                                /v[1]*data->base[i2];
	    ratio /= 12.0;
            fprintf(xfile,"%f %f\n",time,ratio);
	}
	fclose(xfile);

	is = data->num_segs - data->num_backtrace;
	ie = data->num_segs;
	sprintf(fname,"xg/Compare.xg");
	xfile = fopen(fname,"w");
        fprintf(xfile,"color=%s\n",data->assets[i1].color);
        fprintf(xfile,"thickness = 1.5\n");
	for (i = is; i < ie; ++i)
	{
	    time = (double)i;
	    v_ave = 0.5*(data->assets[i1].norm_price[i] +
			     	data->assets[i2].norm_price[i]);
	    fprintf(xfile,"%f  %f\n",time,
				data->assets[i1].norm_price[i]-v_ave);
	}
	if (string[0] == 'y' || string[0] == 'Y')
	{
	    v_ave = 0.5*(v[0]/data->base[i1] +
				v[1]/data->base[i2]);
	    fprintf(xfile,"%f  %f\n",time+1,
				v[0]/data->base[i1]-v_ave);
	}
        fprintf(xfile,"\nNext\n");
        fprintf(xfile,"color=%s\n",data->assets[i2].color);
        fprintf(xfile,"thickness = 1.5\n");
	for (i = is; i < ie; ++i)
	{
	    time = (double)i;
	    v_ave = 0.5*(data->assets[i1].norm_price[i] +
			     	data->assets[i2].norm_price[i]);
	    fprintf(xfile,"%f  %f\n",time,
				data->assets[i2].norm_price[i]-v_ave);
	}
	if (string[0] == 'y' || string[0] == 'Y')
	{
	    v_ave = 0.5*(v[0]/data->base[i1] +
				v[1]/data->base[i2]);
	    fprintf(xfile,"%f  %f\n",time+1,
				v[1]/data->base[i2]-v_ave);
	}
	fclose(xfile);
	printf("Comparing two assets:\n");
        printf("%d  %4s  color=%s\n",i1,data->assets[i1].asset_name,
				data->assets[i1].color);
        printf("%d  %4s  color=%s\n",i2,data->assets[i2].asset_name,
				data->assets[i2].color);
}	/* end InitTrade */

extern void PrintEquityIndex(MARKET_DATA *data)
{
	int i;
	int M = data->num_assets;
	for (i = 0; i < M/3; ++i)
	{
	    printf("%5s %2d   %5s %2d   %5s %2d\n",
			data->assets[3*i].asset_name,3*i,
			data->assets[3*i+1].asset_name,3*i+1,
			data->assets[3*i+2].asset_name,3*i+2);
	}
	for (i = 3*(M/3); i < M; ++i)
	    printf("%5s %2d   ",data->assets[i].asset_name,i);
	printf("\n");
}	/* end PrintEquityIndex */

extern void PrintAccountValue(
	PORTFOLIO *account)
{
	MARKET_DATA *data = account->data;
	int i,M,n;
	double market_value = 0.0;
	char string[100];
	M = data->num_assets;
	n = data->num_segs-1;
	for (i = 0; i < M; ++i)
	{
	    market_value += account->shares[i]*data->assets[i].price[n];
	}
	printf("Total account market value: %f\n",market_value);
	printf("Type yes to print shares: ");
	scanf("%s",string);
	if (string[0] == 'y')
	{
	    for (i = 0; i < M; ++i)
	    	printf("%4s:  %d\n",data->assets[i].asset_name,
				account->shares[i]);
	}
}	/* end PrintAccountValue */

static double AssetTimeIntegral(
	MARKET_DATA *data,
	int index,
	int istart,
	int iend)
{
	int i,j;
	double I = 0.0;

	for (i = istart+1; i < iend-1; ++i)
	    I += data->assets[index].price[i]; 
	I += 0.5*(data->assets[index].price[istart] +
		  data->assets[index].price[iend-1]);
	return I;
}	/* end AssetTimeIntegral */

extern MARKET_DATA *CopyMarketData(
	MARKET_DATA *data)
{
	int i,j;
	MARKET_DATA *copy_data;
	double ave_value;
	
	FT_ScalarMemoryAlloc((POINTER*)&copy_data,sizeof(MARKET_DATA));
	copy_data->num_assets = data->num_assets;
	copy_data->num_segs = data->num_segs;
	copy_data->num_backtrace = data->num_backtrace;
	FT_VectorMemoryAlloc((POINTER*)&copy_data->assets,data->num_assets+1,
				sizeof(ASSET));
	FT_VectorMemoryAlloc((POINTER*)&copy_data->base,data->num_assets+1,
				sizeof(double));
	for (j = 0; j < data->num_assets+1; ++j)
	{
	    memcpy(copy_data->assets[j].asset_name,data->assets[j].asset_name,
				strlen(data->assets[j].asset_name)+1);
	    memcpy(copy_data->assets[j].color,data->assets[j].color,
				strlen(data->assets[j].color)+1);
	    copy_data->base[j] = data->base[j];
	    FT_VectorMemoryAlloc((POINTER*)&copy_data->assets[j].price,
				data->num_segs+10,sizeof(double));
	    FT_VectorMemoryAlloc((POINTER*)&copy_data->assets[j].norm_price,
				data->num_segs+10,sizeof(double));
	}
	for (i = 0; i < data->num_segs; ++i)
	{
	    	for (j = 0; j < data->num_assets+1; ++j)
		{
		    copy_data->assets[j].price[i] = data->assets[j].price[i];
		    copy_data->assets[j].norm_price[i] = 
			  data->assets[j].price[i]/data->base[j];
		    ave_value += data->assets[j].norm_price[i];
		}
		ave_value = 0.0;
	    	for (j = 0; j < data->num_assets; ++j)
		    ave_value += data->assets[j].norm_price[i];
		copy_data->assets[j].price[i] = ave_value/data->num_assets;
		copy_data->assets[j].norm_price[i] = ave_value/data->num_assets;
	}
	return copy_data;
}	/* end CopyMarketData */

extern void FreeMarketData(
	MARKET_DATA *data)
{
	int i,j;
	for (j = 0; j < data->num_assets+1; ++j)
	    FT_FreeThese(2,data->assets[j].price,data->assets[j].norm_price);
	FT_FreeThese(1,data->assets);
	FT_FreeThese(1,data);
}	/* end FreeMarketData */

extern void TimelyRecordMarketData(
	MARKET_DATA *data)
{
	int i,j,m,M;
	char string[100];
	double ave_value;
	char **stock_names;
	double *current_price;
	std::time_t t;
	int day,hour,minute;
	char data_name[256];

	M = data->num_assets;
	FT_VectorMemoryAlloc((POINTER*)&stock_names,M,sizeof(char*));
	FT_VectorMemoryAlloc((POINTER*)&current_price,M,sizeof(double));
	strcpy(data_name,data->data_name);

	while (YES)
	{
	    t = std::time(NULL);
	    strftime(string,100*sizeof(char),"%u",std::localtime(&t));
	    day = atoi(string);
	    strftime(string,100*sizeof(char),"%H",std::localtime(&t));
            hour = atoi(string);
	    m = data->num_segs;
	    if (day >= 1 && day <= 5 && hour >= 10 && hour <= 16)
	    {
		FreeMarketData(data);
		FT_ScalarMemoryAlloc((POINTER*)&data,sizeof(MARKET_DATA));
		strcpy(data->data_name,data_name);
		ReadMarketData(data);
	    	for (j = 0; j < M; ++j)
	    	    stock_names[j] = data->assets[j].asset_name;
	    	GetCurrentPrice(stock_names,current_price,M);

	    	ave_value = 0.0;
	    	for (j = 0; j < M; ++j)
	    	{
	    	    data->assets[j].price[m] = current_price[j];
	    	    data->assets[j].norm_price[m] = data->assets[j].price[m]/
					data->base[j];
	    	    ave_value += data->assets[j].norm_price[m];
	    	}
	   	data->base[j] = 1.0;
	    	data->assets[j].price[m] = ave_value/data->num_assets;
	    	data->assets[j].norm_price[m] = data->assets[j].price[m];
	    	data->num_segs += 1;
		data->new_data = YES;
		WriteMarketData(data);
	    	sleep(6000);
	    }
	    sleep(60);
	}
}	/* end TimelyRecordMarketData */

static void DeleteOldData(
	MARKET_DATA *data)
{
	int i,nd;
	printf("Current number of segments is: %d\n",data->num_segs);
	printf("Enter number of old segments to be deleted: ");
	scanf("%d",&nd);
	data->num_segs -= nd;
	for (i = 0; i < data->num_assets; ++i)
	{
	    data->assets[i].price += nd;
	    data->assets[i].norm_price += nd;
	}
	data->new_data = YES;
}	/* end DeleteOldData */

extern void ComputeAllNormPrice(
        MARKET_DATA *data,
	int order)
{
	int i;
	int num_segs = data->num_segs;

	if (order > 2) order = 2;
	for (i = 0; i < num_segs; ++i)
	    ComputeNormalPrice(data,i,order);
}	/* end ComputeAllNormPrice */

extern void LeastSquareConstant(
	double *x,
	double *y,
	int N,
	double *C)
{
	int i;
	double I = 0.0;
	for (i = 0; i < N; ++i)
	    I += y[i];
	I -= 0.5*(y[0] + y[N-1]);
	*C = I/N;
}	/* end AssetTimeIntegral */
