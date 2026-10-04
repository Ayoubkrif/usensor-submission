#include "BatchQueue.hpp"

// push : appelé par le parser SEULEMENT
// prend le MX
// TANT QUE la file est pleine : dort sur _notFull
// ecrase le lot dans la première case libre
// lâche le MX, réveille le solver _notEmpty
void	BatchQueue::push(const Batch &batch)
{
}

// pop : appelé par le solver SEULEMENT
// reserve avec le MX
// TANT QUE la file est vide : lâche le MX et dort sur _notEmpty
// au réveil le prend le : copie le lot de tête dans son propre batch
// avance head modulo CAPACITY, et decremente _size
// lâche le MX, réveille le parser _notFull
void	BatchQueue::pop(Batch &batch)
{
}
