#pragma once

class OtherUtils
{
public:
    static int getAverage(int data[], int size)
    {
        int sum = 0;
        for (int i = 0; i < size; i++)
        {
            sum += data[i];
        }
        return size > 0 ? sum / size : 0;
    }
};
