////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 
 
 LICENSE:
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */


/* created by the OWL team 2013 */


////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SimpleDelayPatch_hpp__
#define __SimpleDelayPatch_hpp__

#include "Patch.h"

#define SIMPLE_DELAY_REQUEST_BUFFER_SIZE 1024*128


// CircularBuffer code

class CircularBuffer {
private:
  FloatArray buffer;
  unsigned int writeIndex;
public:
  CircularBuffer() : writeIndex(0) {
  }
  CircularBuffer(float* buf, int size) : buffer(buf, size), writeIndex(0) {
  }

  /** 
   * write to the tail of the circular buffer 
   */
  inline void write(float value){
    if(++writeIndex == buffer.getSize())
      writeIndex = 0;
    buffer[writeIndex] = value;
  }

  /**
   * read the value @param index steps back from the head of the circular buffer
   */
  inline float read(int index){
    return buffer[(writeIndex + (~index)) & (buffer.getSize()-1)];
  }

  /**
   * get the value at the head of the circular buffer
   */
  inline float head(){
    return buffer[(writeIndex - 1) & (buffer.getSize()-1)];
  }

  /** 
   * get the most recently written value 
   */
  inline float tail(){
    return buffer[(writeIndex) & (buffer.getSize()-1)];
  }

  /**
   * get the capacity of the circular buffer
   */
  inline unsigned int getSize(){
    return buffer.getSize();
  }

  /**
   * return a value interpolated to a floating point index
   */
  inline float interpolate(float index){
    int idx = (int)index;
    float low = read(idx);
    float high = read(idx+1);
    float frac = index - idx;
    return low*frac + high*(1-frac);
  }

  FloatArray getSamples(){
    return buffer;
  }

  static CircularBuffer* create(int samples){
    CircularBuffer* buf = new CircularBuffer();
    buf->buffer = FloatArray::create(samples);
    return buf;
  }

  static void destroy(CircularBuffer* buf){
    FloatArray::destroy(buf->buffer);
  }
};

// end CircularBuffer code



class DelayPatch : public Patch {
private:
  CircularBuffer* delayBuffer;
  int delay;
  float alpha, dryWet;
public:
  DelayPatch() : delay(0), alpha(0.04), dryWet(0.f)
  {
    registerParameter(PARAMETER_A, "Delay");
    registerParameter(PARAMETER_B, "Feedback");
    registerParameter(PARAMETER_C, "");
    registerParameter(PARAMETER_D, "Dry/Wet");
    delayBuffer = CircularBuffer::create(SIMPLE_DELAY_REQUEST_BUFFER_SIZE);
  }
  void processAudio(AudioBuffer &buffer)
  {
    float delayTime, feedback, dly;
    delayTime = 0.05+0.95*getParameterValue(PARAMETER_A);
    feedback  = getParameterValue(PARAMETER_B);
    int32_t newDelay;
    newDelay = alpha*delayTime*(delayBuffer->getSize()-1) + (1-alpha)*delay; // Smoothing
    dryWet = alpha*getParameterValue(PARAMETER_D) + (1-alpha)*dryWet;       // Smoothing
      
    float* x = buffer.getSamples(0);
    int size = buffer.getSize();
    for (int n = 0; n < size; n++)
    {
      dly = (delayBuffer->read(delay)*(size-1-n) + delayBuffer->read(newDelay)*n)/size;
      delayBuffer->write(feedback * dly + x[n]);
      x[n] = dly*dryWet + (1.f - dryWet) * x[n];  // dry/wet
    }
    delay=newDelay;
  }
};

#endif // __SimpleDelayPatch_hpp__

