#ifndef _STATETASK_H_
#define _STATETASK_H_

class StateTask
{
  public:
    bool mIsActive{};
    double mStaTimeS{};
    double mEndTimeS{};
    double mCurTimeS{};

    void Activate(double curTimeS, double lenTimeS)
    {
        if (!mIsActive)
        {
            mIsActive = true;
            mStaTimeS = curTimeS;
            mEndTimeS = curTimeS + lenTimeS;
            mCurTimeS = curTimeS;
        }
    }

    void Deactivate() { mIsActive = false; }

    bool IsActive() const { return mIsActive; }

    void AnimStateTask(double curTimeS)
    {
        if (!mIsActive) return;

        if (curTimeS >= mEndTimeS)
        {
            mIsActive = false;
            mCurTimeS = mEndTimeS;
        }
        else mCurTimeS = curTimeS;
    }

    double GetStateT() const
    {
        const auto d = mEndTimeS - mStaTimeS;
        return d != 0 ? (mCurTimeS - mStaTimeS) / d : 1;
    }

    float GetStateTf() const { return (float)GetStateT(); }
};

#endif
