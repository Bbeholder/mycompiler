int a[6];
int dp[10][10];
int main()
{
	int n;
	n=6;

	a[0] = 10;
	a[1] = 1;
	a[2] = 50;
	a[3] = 50;
	a[4] = 20;
	a[5] = 5;
	
	int k;
	int i;
	int t;
	int j;
	int aa;
	k=3;
	while(k<=n)
	{
		i=0;
		while(i<n-k+1)
		{
			j=i+k-1;
			t=i+1;
			while(t<j)
			{
				aa= dp[i][t]+dp[t][j]+a[i]*a[t]*a[j];
				if(!dp[i][j]||aa<dp[i][j])
				{
					dp[i][j]=aa;
				}
				t=t+1;
			}
			i=i+1;
		}
		k=k+1;
	}
	return (dp[0][n-1]);
}
