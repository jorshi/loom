/*
  ==============================================================================

    SinusoidalSynthVoice.cpp
    Created: 17 Feb 2017 4:03:11pm
    Author:  Jordie Shier 

  ==============================================================================
*/

#include "SinusoidalSynthVoice.h"


// Default Constructor
SinusoidalSynthVoice::SinusoidalSynthVoice()
:
hopSize_(128),
frameSize_(512),
overlapIndex_(0),
location_(0.0)
{
    hopIndex_ = hopSize_;
    writePos_ = 0;
    readPos_ = 0;
    nyquistBin_ = frameSize_ / 2;
    
    // Inverse FFT of frame size
    inverseFFT_ = new FFT(std::log2(frameSize_), true);
    
    spectrum_.resize(frameSize_);
    timeDomain_.resize(frameSize_);
    
    output_.create(frameSize_);
    buffer_.create(frameSize_);
}

// Deconstructor
SinusoidalSynthVoice::~SinusoidalSynthVoice()
{
}

bool SinusoidalSynthVoice::canPlaySound (SynthesiserSound* sound)
{
    return dynamic_cast<const SinusoidalSynthSound*> (sound) != nullptr;
}

void SinusoidalSynthVoice::startNote (const int midiNoteNumber,
                                      const float velocity,
                                      SynthesiserSound* s,
                                      const int /*currentPitchWheelPosition*/)
{
    if (const SinusoidalSynthSound* const sound = dynamic_cast<const SinusoidalSynthSound*> (s))
    {
        location_ = 0.0;
        readPos_ = 0;
        writePos_ = 0;
        hopIndex_ = hopSize_;
    }
    else
    {
        jassertfalse;
    }
}

void SinusoidalSynthVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    hopIndex_ = hopSize_;
    overlapIndex_ = 0;
    location_ = 0.0;
    readPos_ = 0;
    clearCurrentNote();
}

void SinusoidalSynthVoice::pitchWheelMoved (const int /*newValue*/)
{
}

void SinusoidalSynthVoice::controllerMoved (const int /*controllerNumber*/,
                                            const int /*newValue*/)
{
}

//==============================================================================
void SinusoidalSynthVoice::renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
{
    
    if (const SinusoidalSynthSound* const playingSound = static_cast<SinusoidalSynthSound*> (getCurrentlyPlayingSound().get()))
    {
        
        float* outL = outputBuffer.getWritePointer (0, startSample);
        float* outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer (1, startSample) : nullptr;
        
        int numCalculated = 0;
        
        
        while (numCalculated < numSamples)
        {
            // Get a new frame
            if (hopIndex_ >= hopSize_)
            {
                if (renderFrames(output_, playingSound))
                {
                    // Do overlap add
                    for (int i = 0; i < playingSound->getFrameSize(); ++i)
                    {
                        if (i < hopSize_)
                        {
                            buffer_((writePos_ + i) % playingSound->getFrameSize()) = 0.0;
                        }
                        else
                        {
                            buffer_((writePos_ + i) % playingSound->getFrameSize()) += output_(i);
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < hopSize_; ++i)
                    {
                        buffer_(writePos_ + i) = 0.0;
                    }
                    clearCurrentNote();
                }
                
                // Update write pointer and read pointer
                writePos_ = (writePos_ + hopSize_) % playingSound->getFrameSize();
                readPos_ = (readPos_ + hopSize_) % playingSound->getFrameSize();
                location_ += (hopSize_ / getSampleRate());
                hopIndex_ = 0;
            }
            else
            {
                *(outL + numCalculated) += buffer_(readPos_ + hopIndex_);
                *(outR + numCalculated) += buffer_(readPos_ + hopIndex_);
                numCalculated++;
                hopIndex_++;
            }
        }
    }
}

//==============================================================================
bool SinusoidalSynthVoice::renderFrames(mrs_realvec &buffer, const SinusoidalSynthSound* const sound)
{
    ReferenceCountedArray<SineModel> activeModels = sound->getPlayingSineModels();
    if (activeModels.size() < 1)
    {
        return false;
    }
    
    SineModel::ConstPtr model = activeModels[0];
    
    // Get number of frames in the model return if there aren't any
    int modelFrames = std::distance(model->begin(), model->end());
    if (modelFrames < 1)
    {
        return false;
    }
    
    // Get the frame closest to the requested time
    mrs_real requestedPos = (location_ * model->getSampleRate()) / model->getFrameSize();
    int requestedFrame = std::round(requestedPos);
    
    // Out of frames from the model
    if (modelFrames <= requestedFrame)
    {
        return false;
    }
    
    // Constant reference to the frame at this point
    const SineModel::SineFrame& frame = model->getFrame(requestedFrame);
    
    // Zero out spectrum
    std::fill(spectrum_.begin(), spectrum_.end(), FFT::Complex());
    
    // Declare some variables for use in processing loop
    mrs_real freq;
    mrs_real binLoc;
    mrs_natural binInt;
    mrs_real binRem;
    
    mrs_real mag;
    mrs_real phase;
    
    // Create the spectral signal
    for (auto sine = frame.begin(); sine != frame.end(); ++sine)
    {
        // Do frequency transformations here
        freq = sine->getFreq();
        
        // Voice level frequency scaling (if quantized to midi pitch)
        
        // Sound level frequency scaling ( get from the 
        
        //freq *= 0.5;
        
        binLoc =  (freq / getSampleRate()) * frameSize_;
        binInt = std::round(binLoc);
        binRem = binInt - binLoc;
        
        // Convert the decibels back to magnitude
        mag = pow(10, sine->getAmp()/20);
        phase = sine->getPhase();
        
        // Going to make a 9 bin wide Blackman Harris window
        if (binLoc >= 5 && binLoc < nyquistBin_-4)
        {
            for (int i = -4; i < 5; ++i)
            {
                spectrum_.at(binInt + i).r += mag*sound->getBH((int)((binRem+i)*100) + 501)*cos(phase);
                spectrum_.at(binInt + i).i += mag*sound->getBH((int)((binRem+i)*100) + 501)*sin(phase);
            }
        }
        // Some components will wrap around 0
        else if (binLoc < 5 && binLoc > 0)
        {
            for (int i = -4; i < 5; ++i)
            {
                // Complex Conjugate wraps around DC bin
                if ((binInt + i) < 0)
                {
                    spectrum_.at(-1*(binInt + i)).r += mag*sound->getBH((int)((binRem+i)*100) + 501)*cos(phase);
                    spectrum_.at(-1*(binInt + i)).i += -mag*sound->getBH((int)((binRem+i)*100) + 501)*sin(phase);
                }
                // Real only at DC bin
                else if ((binInt + i) == 0)
                {
                    spectrum_.at(0).r += 2*mag*sound->getBH((int)((binRem+i)*100) + 501)*cos(phase);
                }
                // Regular
                else
                {
                    spectrum_.at(binInt + i).r += mag*sound->getBH((int)((binRem+i)*100) + 501)*cos(phase);
                    spectrum_.at(binInt + i).i += mag*sound->getBH((int)((binRem+i)*100) + 501)*sin(phase);
                }
            }
        }
        // Wrap around the Nyquist bin
        else if (binLoc < (nyquistBin_ - 1) && binLoc >= (nyquistBin_-4))
        {
            for (int i = -4; i < 5; ++i)
            {
                // Complex Conjugate wraps nyquist bin
                if ((binInt + i) > nyquistBin_)
                {
                    spectrum_.at((binInt + i) - nyquistBin_).r +=
                    mag*sound->getBH((int)((binRem+i)*100) + 501)*cos(phase);
                    spectrum_.at((binInt + i) - nyquistBin_).i +=
                    -mag*sound->getBH((int)((binRem+i)*100) + 501)*sin(phase);
                }
                // Real only at Nyquist
                else if ((binInt + i) == nyquistBin_)
                {
                    spectrum_.at(nyquistBin_).r += 2*mag*sound->getBH((int)((binRem+i)*100) + 501)*cos(phase);
                }
                // Regular
                else
                {
                    spectrum_.at(binInt + i).r += mag*sound->getBH((int)((binRem+i)*100) + 501)*cos(phase);
                    spectrum_.at(binInt + i).i += mag*sound->getBH((int)((binRem+i)*100) + 501)*sin(phase);
                }
            }
        }
    }
    
    // Conjugate for bins above the nyquist frequency
    for (int i = 1; i < nyquistBin_; ++i)
    {
        spectrum_.at(nyquistBin_ + i).r = spectrum_.at(nyquistBin_ - i).r;
        spectrum_.at(nyquistBin_ + i).i = -spectrum_.at(nyquistBin_ - i).i;
    }
    
    inverseFFT_->perform(spectrum_.data(), timeDomain_.data());
    
    // Apply synthesis window & shift
    for (int i = 0; i < frameSize_; ++i)
    {
        buffer(i) = timeDomain_.at((i + (frameSize_ / 2)) % frameSize_).r / frameSize_ * sound->getSynthWindow(i);
    }
    
    return true;
}

