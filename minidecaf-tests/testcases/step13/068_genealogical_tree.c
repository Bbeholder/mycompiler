int map[10][10];
int indegree[10];
int queue[10];
void topo(int n)
{
	int m;
	int t;
	int i;
	int j; 
	m = 0;
	t = 0;
	i=1;
	j=1;
	while(i<=n)
	{
		j=1;
		while(j<=n)
		{
			if(indegree[j]==0)
			{
				
				m=j;
				break;
			}
			j=j+1;
		}
		queue[t]=m;
		t=t+1;
		indegree[m]=-1;
		j=1;
		while(j<=n)
		
		{
			if(map[m][j])
			{
				indegree[j]=indegree[j]-1;
			 } 
			j=j+1;
		}
		i=i+1;
	}
	i=0;
	while(i<n)
	{
		putint(queue[i]); 
		putch(10);
		i=i+1; 
	}
		
}
int main()
{
	int n;
	int p;
	int i;
	i=1;
	n=5;

	while(i<=n)
	{
		p=getint();
		while(p!=0)
		{
			map[i][p]=1;
			indegree[p]=indegree[p]+1;
			p=getint();
			
		}
		i=i+1;
	}
	topo(n);
	return 0;
}
