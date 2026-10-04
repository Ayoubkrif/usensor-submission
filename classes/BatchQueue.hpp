#pragma once

# include "Event.hpp"
# include <condition_variable>
# include <cstddef>
# include <mutex>

// 50 events car 100ms a 400hz = 40 events
constexpr size_t	BATCH_SIZE = 50;

// Lot d'events triés.
// Last marque le dernier lot du flux (EOF)
// count peut y valoir 0.
struct Batch
{
	Event	events[BATCH_SIZE];
	size_t	count;
	bool	last;
};

// File de lots circulaire entre le parser et le solver.
// protege par un MX
class BatchQueue
{
	public:
		// Bloque TANTQUE la file est PLEINE.
		void	push(const Batch &batch);
		// Bloque TANTQUE la file est VIDE.
		void	pop(Batch &batch);

	private:
		static constexpr size_t	CAPACITY = 4;

		Batch					_ring[CAPACITY];
		size_t					_currentHead = 0;
		size_t					_currentSize = 0;
		std::mutex				_mutex;
		std::condition_variable	_notEmpty;
		std::condition_variable	_notFull;
};
