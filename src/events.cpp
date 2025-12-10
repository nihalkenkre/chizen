#include "events.hpp"
#include "utils.hpp"

Events events = {};

void Events_Initialize()
{
	size_t events_count = 0;
	for (
		uint8_t* e = reinterpret_cast<uint8_t*>(&events);
		e < reinterpret_cast<uint8_t*>(&events) + sizeof(events);
		e += sizeof(SDL_Event))
	{
		++events_count;
	}

	uint32_t id = SDL_RegisterEvents(static_cast<int>(events_count));
	SDL_CHECK(id);

	for (
		uint8_t* e = reinterpret_cast<uint8_t*>(&events);
		e < reinterpret_cast<uint8_t*>(&events) + sizeof(events);
		e += sizeof(SDL_Event))
	{
		reinterpret_cast<SDL_Event*>(e)->type = id++;
	}
}
