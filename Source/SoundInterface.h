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
#include "StochasticAnalysis.h"
#include "SineElement.h"
#include "AnalysisParameterManager.h"
#include "SynthesisParameterManager.h"
#include "ParameterManager.h"
#include "FileLoader.h"
#include "ModulationFactory.h"

class SoundInterface
{
public:

    enum
    {
        visualizerSize = 512
    };

    // Current state of this interface
    enum State
    {
        loadFileState,
        analysisState,
        runningAnalysisState,
        synthesisState
    };
    
    // Default Constructor
    SoundInterface(AnalysisParameterManager* a, SynthesisParameterManager* s);
    
    // Default Deconstructor
    ~SoundInterface();
    
    // Create a new analysis object
    void buildAnalysis(File inputFile);
    
    // Run Analysis on current analysis
    void runAnalysis();
    
    // Sine Model Getter
    SineModel::ConstPtr getSineModel() const { return currentSineModel_; };
    
    // Stochastic Model Getter
    StochasticModel::ConstPtr getStochasticModel() const { return currentStochasticModel_; };
    
    // Get current state of sound
    State getState() const { return state_; };
    
    // Set current state of sound
    void setState(State newState) { state_ = newState; };
    
    // Get a parameter value from a parameter name
    float getParameterValue(String);

    // Get a pointer to the analysis parameters for this model
    AnalysisParameterManager* getAnalysisParams() const { return analysisParams_; };
    
    // Get a pointer to the synthesis parameters for this model
    SynthesisParameterManager* getSynthParams() const { return synthParams_; };
    
    // Load audio from file and load into a new analysis object
    void loadFile();
    void loadFile(const String& fileName);
    
    // Pre-load check to see if a filename looks okay
    bool willAcceptFile(const String& fileName);
    
    // Whether or not this sound can be played
    bool isActive() { return isActive_; };
    void setActive(bool val) { isActive_ = val; };
    
    // Checks reference counted SineModels to see if any can be freed
    void checkModels();
    
    void addVisualizerFrame(float freq, float amp);
    void setVisualize(bool value) { canVisualize = value; };
    bool getVisualize() const { return canVisualize; };
    
    const std::vector<float>& getVisualizerFrame() const { return visualizerFrame_; };
    void clearVisualizerFrame();
    
    
    static void setRate(float rate) { sampleRate_ = rate; };
    


private:
    

    
    ScopedPointer<AnalysisMrs> analysis_;
    ScopedPointer<StochasticAnalysis> stochasticAnalysis_;
    
    // Reference counted pointers to models
    ReferenceCountedArray<SineModel> sineModels_;
    SineModel::Ptr currentSineModel_;
    
    ReferenceCountedArray<StochasticModel> stochasticModels_;
    StochasticModel::Ptr currentStochasticModel_;
    
    // For loading new sounds into the analysis
    FileLoader fileLoader_;
    
    // Parameters for the analysis phase
    AnalysisParameterManager* analysisParams_;
    
    // Parameters for synthesis
    SynthesisParameterManager* synthParams_;
    
    // Current state of this sound
    State state_;
    
    // Can this sound be synthesized
    bool isActive_;
    
    std::vector<float> visualizerFrame_;
    
    bool canVisualize;
    
    static float sampleRate_;
};


#endif  // SoundInterface_H_INCLUDED
