#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H

#define RING_BUFFER_MAX_COUNT 256u
#define RING_BUFFER_TYPE EventType

typedef struct RingBuffer
{
	RING_BUFFER_TYPE elements[RING_BUFFER_MAX_COUNT];
	u32 readPosition;
	u32 writePosition;
} RingBuffer;

function void 
ResetRingBuffer(RingBuffer* ringBuffer)
{
	ringBuffer->readPosition = 0u;
	ringBuffer->writePosition = 0u;
}

function u32
EventCount(RingBuffer* ringBuffer)
{
	return ringBuffer->writePosition - ringBuffer->readPosition;
}

function void
AssertInvariants(RingBuffer* ringBuffer)
{
	b32 first = (s32)(EventCount(ringBuffer)) >= 0; // Read never ahead of write.
	b32 second = (EventCount(ringBuffer)) <= RING_BUFFER_MAX_COUNT; // Max size.

	//second = true; // For overwriting old events when new events arrive, if we want to.
	Assert(first && second);
}


function void 
EnqueueEvent(RingBuffer* ringBuffer, RING_BUFFER_TYPE element)
{
	AssertInvariants(ringBuffer);

	if(EventCount(ringBuffer) < RING_BUFFER_MAX_COUNT)
	{
		ringBuffer->elements[(ringBuffer->writePosition++) & (RING_BUFFER_MAX_COUNT - 1)] = element;
	}

	AssertInvariants(ringBuffer);
}

function void
DequeueEvent(RingBuffer* ringBuffer)
{
	AssertInvariants(ringBuffer);

	Assert(EventCount(ringBuffer) > 0);
	ringBuffer->readPosition++;
	Assert(EventCount(ringBuffer) >= 0);

	AssertInvariants(ringBuffer);
}

function RING_BUFFER_TYPE
GetCurrent(RingBuffer* ringBuffer)
{
	AssertInvariants(ringBuffer);

	Assert(EventCount(ringBuffer) > 0);

	RING_BUFFER_TYPE result = ringBuffer->elements[ringBuffer->readPosition & (RING_BUFFER_MAX_COUNT-1)];

	AssertInvariants(ringBuffer);

	return result;
}

function RING_BUFFER_TYPE
GetNext(RingBuffer* ringBuffer)
{
	AssertInvariants(ringBuffer);

	RING_BUFFER_TYPE result = GetCurrent(ringBuffer);
	DequeueEvent(ringBuffer);

	AssertInvariants(ringBuffer);

	return result;
}

#endif
