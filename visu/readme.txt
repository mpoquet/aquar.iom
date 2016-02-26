Commandes de la visualisation

I : Change la couleur d'arrière-plan de noir à blanc et vice-versa
N : Active/désactive l'affichage des cellules neutres
+ : Zoom avant (facteur/0.75)
- : Zoom arrière (facteur 0.75)
= : Remet le zoom comme au démarrage (carte entière et centrée)
Flèches directionnelles : Déplacent la vue

Lorsqu'un tour est reçu, avant de mettre à jour les cellules, on parcourt toutes les cellules existantes et on les marque comme mortes.
Puis on parcourt les cellules envoyées par la structure tour en les cherchant dans les cellules existantes (pour pouvoir les créer si ce sont des nouvelles cellules). Pour chaque cellule ainsi traitée, on la marque comme vivante.
A la fin de ce parcours, toutes les cellules existantes qui sont encore marquées comme mortes sont des cellules qu'on n'a pas reçues dans la structure du tour, donc ce sont des cellules qui sont mortes.
Lorsqu'on appelle la fonction d'affichage des cellules, pour chaque cellule, si la cellule est morte, alors on la supprime sans l'afficher.
Ce n'est pas logique de supprimer les cellules mortes dans la fonction d'affichage mais ça permet d'économiser un parcours de l'ensemble des cellules.