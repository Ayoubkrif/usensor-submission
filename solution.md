# resolution possible
1 worker parser checker
1 worker solver

## Objets disponibles :
 - stdin
 - liste de reordonance
 - file d'event {[MX]} (consommateur)
 - liste d'event exploitable
 - file de boutons a resoudre
 - stdout

{[MX]} : mutex. La file est le SEUL objet partagé.

## Workers :
### parser checker
 - stdin
 - tampon a reordonner
 - file d'event trie {[MX]} (producteur)
### solver
 - file d'event trie {[MX]} (consommateur)
 - historique d'event encore exploitable
 - bouton courant
 - file de pending boutons
 - stdout

## Structures
### event
 - event valide recupere sur la file avec timestamp + metriques
### file d'events
 - remplie par le parser au fur et a mesure
 - videe par le solver au fur et a mesure
 - SOLVER:
    - Si pas de bouton courant et pas de bouton pending, vide a la recherche d'un bouton mais rempli historique de capteurs si pas de match
    - si bouton mais historique insufisant, vide a la recherche du capteur demande mais rempli l'historique de capteurs si pas de match.
    (plein de probleme a cette solution mais on la garde)
### historique par capteur
 - rempli a chaque fois que trouve sur la file
### boutons en attente
 - pas de boutons a solve ? checke ici en priorite et se rabat sur la file ensuite
 - rempli a chaque fois que trouve sur la file et si un bouton est deja en traitement
 - petite file triee

## Deroulement :
### parser checker
- lit et valide le header
- boucle :
    - fread() par gros blocs recolle les lectures pas finies modulo 32 pour le tour suivant
    - FOREACH record complet:
        - CONTINUE si header KO
        - décode les champs, pousse dans le tampon
    - FOREACH event dans tampon
        - determine min
        - determine max
        - si 2ms ou plus de differenc eentre min et max push min dans le lot
    - reserve un lot de la file 
    - publie le lot (lock)
    - SI SYSTEME DE LOTS TROP COMPLIQUE alors just emutex sur la file
- ATTENTION A not_full :
    - EVENT avec flag fin ?
    LOOP

### solver
- boucle :
    - attend un lot (not_empty), le prend,
    - pour chaque record du lot :
        - remplit les differentes liste historique (sauf temperature car useless et yaw invalide et buttons pas presses) + boutons pending
        - edite current_ts avec la metrique
    -tant qu'il ya des boutons a solve :
        - si le premier bouton en attente a T + 100 ms > current_ts
            - attend le prochain lot
        - pop le bouton
        - cherche les metriques a print
            - trouve la meilleure metrique pour chaque, pop les metriques pas utilisable (les metriques ecrasees ?)
            - formate le JSON dans un buffer et l'écrit
    - SI lot final : émet tous les boutons restants, flush stdout, stop
- main : lance le parser, fait le solver, join le parser
