bool detect_deadlock(int allocation[][m], int request[][m], int available[], int n, int m)
{
    bool finish[n] = {false};
    int work[m];

    for (int j=0; j<m; j++)
    {
        work[j] = available[j];
    }

    //Banker's Algorithm
    bool made_progess;
    do
    {
        made_progess = false;
        for (int i=0; i<n; i++)
        {
            if (!finish[i])
            {
                bool can_proceed = true;
                for (int j=0; j<m; j++)
                {
                    if (request[i][j] > work[j]) //inversion of banker's algorithm, so use request > available as a fail case rather than request <= available as a pass case.
                    {
                        can_proceed = false;
                        break;
                    }
                }
                if (can_proceed)
                {
                    for (int j=0; j<m; j++)
                    {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    made_progess = true;
                }
            }
        }
    } while (made_progess);

    //unfinished process detection
    for (int i=0; i<n; i++)
    {
        if (!finish[i]) return true;//deadlock
    }
    reutrn false;//no deadlock
}