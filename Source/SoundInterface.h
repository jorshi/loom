/*
  ==============================================================================

    SoundInterface.h
    Created: 18 Mar 2017 12:28:33pm
    Author:  Jordie Shier 
 
    This should store information related to an sound -- analysis and synthesis

  ==============================================================================
*/

#ifndef SoundInterface_H_INCLUDED
#define SoundInterface_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "AnalysisMrs.h"
#include "SineElement.h"
#include "AnalysisParameterManager.h"
#include "ParameterManager.h"

class SoundInterface
{
public:

    // Current state of this interface
    enum State
    {
        loadFileState,
        analysisState,
        runningAnalysisState,
        synthesisState
    };
    
    // Default Constructor
    SoundInterface(AnalysisParameterManager* a);
    
    // Default Deconstructor
    ~SoundInterface();
    
    // Create a new analysis object
    void buildAnalysis(File inputFile);
    
    // Run Analysis on current analysis
    void runAnalysis();
    
    // Sine Model Getter
    const SineModel& getSineModel() const { return *sineModel_; };
    
    // Get current state of sound
    State getState() const { return state_; };
    
    // Set current state of sound
    void setState(State newState) { state_ = newState; };
    
    float getParameterValue(String);
    
    AnalysisParameterManager* getAnalysisParams() { return analysisParams_; };
    
private:
    
    ScopedPointer<AnalysisMrs> analysis_;
    ScopedPointer<SineModel> sineModel_;
    
    State state_;
    
    // Parameters for the analysis phase
    AnalysisParameterManager* analysisParams_;
    
};


#endif  // SoundInterface_H_INCLUDED