#include "visu.hpp"

Visu::Visu()
{
    // initialisation des attributs : pour la plupart il faut attendre un Welcome pour avoir les données
    window_height = 768;
    window_width = 1024;
    background_color = sf::Color::Black;
    borders_color = sf::Color::White;

    afficheCellulesNeutres = false; // par défaut les cellules neutres ne sont pas affichées dans la fenêtre
    partieEnCours = true;
    tourCourant = 0;

    cadre.setFillColor(background_color);
    cadre.setOutlineColor(borders_color);
    cadre.setOutlineThickness(1);

    // création de la fenêtre
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    window.create(sf::VideoMode(window_width, window_height), "Aquar.iom", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(false);
    window.setKeyRepeatEnabled(true); // activer/désactiver la répétition des touches si on les maintient appuyées
    window.setFramerateLimit(30);

    droite.reset(sf::FloatRect(0.85*window_width, 0, 0.15*window_width, window_height));
    droite.setViewport(sf::FloatRect(0.85, 0, 0.15, 1));

    bas.reset(sf::FloatRect(0, 0.85*window_height+1, window_width, 0.15*window_height));
    bas.setViewport(sf::FloatRect(0, 0.85, 1, 0.15));

    police.loadFromFile("fonts/F-Zero GBA Text 1.ttf");

}

Visu::~Visu()
{
}

void Visu::onWelcomeReceived(const ainet16::Welcome &welcome)
{
    // enregistrer les paramètres de la partie dans la classe
    parameters = welcome.parameters;

    // initialiser la vue
    vue_carte.reset(sf::FloatRect(0, 0, parameters.map_width, parameters.map_height));
    vue_carte.setViewport(sf::FloatRect(0, 0, 0.85, 0.85));

    /*cadre.setSize(sf::Vector2f(parameters.map_width, parameters.map_height));
    vue_cadre.reset(sf::FloatRect(0, 0, parameters.map_width/0.85, parameters.map_height/0.85));*/
    cadre.setSize(sf::Vector2f(vue_carte.getViewport().width*window_width, vue_carte.getViewport().height*window_height));
    vue_cadre.reset(sf::FloatRect(0, 0, cadre.getSize().x, cadre.getSize().y));
    vue_cadre.setViewport(sf::FloatRect(0, 0, vue_carte.getViewport().width+cadre.getOutlineThickness(), vue_cadre.getViewport().height+cadre.getOutlineThickness()));

    // initialiser l'ensemble des cellules avec leurs positions initiales
    std::vector<ainet16::Position>::const_iterator it;
    quint32 numero = 0;

    for (it=welcome.initial_ncells_positions.begin() ; it!=welcome.initial_ncells_positions.end() ; ++it) {
        // créer la cellule
        const ainet16::Position & pos = *it;

        Cellule* cell = new Cellule(pos, parameters.initial_neutral_cells_mass, numero);

        // l'ajouter dans le conteneur de toutes les cellules
        addNewCell(cell);

        ++numero;
    }
    nbCellulesInitiales = numero;

    sort(allCellsByMass.begin(), allCellsByMass.end(), CompareMasseCellules());

    // pas besoin d'initialiser les joueurs : on va les recevoir dans un Turn
}

void Visu::onTurnReceived(ainet16::Turn &turn, int numeroTour)
{
    tourCourant = numeroTour;

    if (players.size()==0)
    {
        // C'est la première fois qu'on reçoit un Turn : initialisation de la liste des joueurs
        for (unsigned int i=0; i<turn.players.size(); ++i)
            addNewPlayer(turn.players[i]);
    }

    else
    {
        // mettre à jour le score des joueurs
        // Les deux listes sont triées par player_id croissant, afin
        // que les indices des deux listes correspondent
        std::sort(players.begin(), players.end(), CompareIdJoueurs());
        std::sort(turn.players.begin(), turn.players.end(), CompareIdJoueurs());

        for (uint i=0; i<players.size(); ++i)
        {
            players[i].mass = turn.players[i].mass;
            players[i].nb_cells = turn.players[i].nb_cells;
            players[i].score = turn.players[i].score;
        }
    }

    // Partir du principe que toutes les cellules courantes sont mortes.
    // Celles qui sont présentes dans le tour actuel seront marquées comme étant vivantes.
    // À la fin du parcours, supprimer les cellules étant toujours marquées comme étant mortes.
    for (auto mit : allCells)
    {
        Cellule * cell = mit.second;
        cell->estVivante = false;
    }

    // Mise à jour des virus
    for (unsigned int i=0; i<turn.viruses.size(); ++i)
    {
        int cell_id = turn.viruses[i].id;
        // vérifier si allCells[indice] existe déjà
        if (allCells.count(cell_id) == 0)
        {
            // créer la cellule et l'ajouter dans l'ensemble
            Cellule* cell = new Cellule(turn.viruses[i], parameters.virus_mass);
            addNewCell(cell);
        }
        else
        {
            allCells[cell_id]->position.x = turn.viruses[i].position.x;
            allCells[cell_id]->position.y = turn.viruses[i].position.y;
            allCells[cell_id]->estVivante = true;
        }
    }

    // Mise à jour des cellules des joueurs
    for (unsigned int i=0; i<turn.pcells.size(); ++i)
    {
        int cell_id = turn.pcells[i].pcell_id;
        // vérifier si allCells[indice] existe déjà
        if (allCells.count(cell_id) == 0)
        {
            // créer la cellule et l'ajouter dans l'ensemble
            Cellule* cell = new Cellule(turn.pcells[i], players.size());
            addNewCell(cell);
        }
        else
        {
            allCells[cell_id]->position.x = turn.pcells[i].position.x;
            allCells[cell_id]->position.y = turn.pcells[i].position.y;
            allCells[cell_id]->mass = turn.pcells[i].mass;
            allCells[cell_id]->remaining_isolated_turns = turn.pcells[i].remaining_isolated_turns;
            allCells[cell_id]->estVivante = true;
        }
    }

    // Mise à jour des cellules neutres initiales
    // On sait que les cellules initiales neutres ont les numéros de 0 à nbCellulesInitiales - 1
    int incell_id = 0;
    for (unsigned int i=0; i<turn.initial_ncells.size(); ++i)
    {
        // Les cellules initiales neutres ne se déplacent pas
        allCells[incell_id]->remaining_turns_before_apparition = turn.initial_ncells[i].remaining_turns_before_apparition;
        allCells[incell_id]->estVivante = true;
        ++incell_id;
    }

    // Mise à jour des cellules neutres non initiales
    for (unsigned int i=0; i<turn.non_initial_ncells.size(); ++i)
    {
        int ncell_id = turn.non_initial_ncells[i].ncell_id;
        // vérifier si allCells[indice] existe déjà
        if (allCells.count(ncell_id) == 0)
        {
            // créer la cellule et l'ajouter dans l'ensemble
            Cellule* cell = new Cellule(turn.non_initial_ncells[i]);
            addNewCell(cell);
        }
        else
        {
            Cellule * cell = allCells[ncell_id];

            if (cell->player_id() != -1)
                cell->transformToNeutralCell();

            cell->position.x = turn.non_initial_ncells[i].position.x;
            cell->position.y = turn.non_initial_ncells[i].position.y;
            cell->mass = turn.non_initial_ncells[i].mass;
            cell->estVivante = true;
        }
    }

    // Toutes les cellules ont été parcourues.
    // On peut désormais savoir quelles cellules sont vraiment mortes et les
    // supprimer de nos structures de données.
    std::vector<Cellule *> cells_to_remove;
    for (auto mit : allCells)
    {
        Cellule * cell = mit.second;
        if (!cell->estVivante)
            cells_to_remove.push_back(cell);
    }

    for (Cellule * cell : cells_to_remove)
    {
        //printf("Removing pcell (id=%d)\n", cell->id());
        // Removes the cell from data structures
        removeCell(cell->id());
        // Cleans memory
        delete cell;
    }

    // Let all the cells be sorted by ascending mass
    sort(allCellsByMass.begin(), allCellsByMass.end(), CompareMasseCellules());

    // trier les joueurs par score décroissant
    sort(players.begin(), players.end(), CompareScoresJoueurs());
}

void Visu::afficheCellule(Cellule* cellule)
{
    bool is_neutral_cell = (cellule->typeDeCellule == initialNeutral) ||
                           (cellule->typeDeCellule == nonInitialNeutral);

    if (is_neutral_cell &&
        (!afficheCellulesNeutres || cellule->remaining_turns_before_apparition > 0))
        return;

    float rayon = cellule->mass * parameters.radius_factor;


    sf::CircleShape * cercle;

    if (is_neutral_cell)
        cercle = new sf::CircleShape(rayon, 8);
    else
        cercle = new sf::CircleShape(rayon, 64);

    cercle->setFillColor(cellule->color);

    // mettre un contour pour les cellules isolées et les virus
    if ((cellule->remaining_isolated_turns != 0) || (cellule->typeDeCellule == virus))
    {
        cercle->setOutlineColor(borders_color);
        cercle->setOutlineThickness(rayon/6);
        rayon = 5.0/6.0 * rayon; // on change la valeur du rayon car la bordure est rajoutée à l'extérieur du circleshape
        cercle->setRadius(rayon);
    }
    cercle->setPosition(cellule->position.x - rayon, cellule->position.y - rayon); // setPosition prend la position du coin supérieur gauche =(1
    window.draw(*cercle);
    delete cercle;
}

void Visu::afficheToutesCellules() {
    window.setView(vue_carte);

    std::vector<Cellule*>::iterator it;
    for (it=allCellsByMass.begin() ; it!=allCellsByMass.end() ; ++it)
    {
        afficheCellule(*it);
    }
}

void Visu::afficheScore()
{
    // pour cacher les cellules qui dépassent de l'aire de jeu on met un rectangle blanc par-dessus
    window.setView(bas);
    sf::RectangleShape cache_bas;
    cache_bas.setSize(sf::Vector2f(bas.getSize().x, bas.getSize().y+cadre.getOutlineThickness()));
    cache_bas.setPosition(bas.getCenter().x-bas.getSize().x/2, bas.getCenter().y-bas.getSize().y/2 + cadre.getOutlineThickness());
    //std::cout << cache_bas.getSize().x << " " << bas.getSize().y << std::endl;
    cache_bas.setFillColor(sf::Color::White);
    window.draw(cache_bas);

    window.setView(droite);
    sf::RectangleShape cache_droite;
    cache_droite.setSize(sf::Vector2f(droite.getSize().x+cadre.getOutlineThickness(), droite.getSize().y));
    cache_droite.setPosition(droite.getCenter().x-droite.getSize().x/2 + cadre.getOutlineThickness(), droite.getCenter().y-droite.getSize().y/2);
    cache_droite.setFillColor(sf::Color::White);
    window.draw(cache_droite);

    // rectangles pour la représentation du score et de la masse relatifs des joueurs
    sf::RectangleShape rect_score; // rectangle de dimensions (0,0)
    sf::RectangleShape rect_masse;
    float hauteur_rect(1.0/4.0 * ((bas.getSize().y))); // la moitié de la distance qui reste entre le bas du plateau et le bas de la fenêtre
    float largeur_rect_score = 0, largeur_rect_masse = 0;

    //std::cout << window_height*0.15 << " " << bas.getSize().y << " " << 0.85*window_height << " " << vue_carte.getViewport().height << std::endl;
    float abscisse_rect_score(0), ordonnee_rect_score(vue_carte.getViewport().height*window_height + hauteur_rect);
    float abscisse_rect_masse(0), ordonnee_rect_masse(vue_carte.getViewport().height*window_height + 3*hauteur_rect); // todo ajuster la position de l'affichage

    std::vector<ainet16::TurnPlayer>::iterator joueur;
    float somme_scores = 0;
    float somme_masse = 0;
    for (joueur=players.begin(); joueur!=players.end(); ++joueur) {
        somme_scores += (*joueur).score;
        somme_masse += (*joueur).mass;
    }
    float facteur_score = bas.getSize().x / somme_scores;
    float facteur_masse = bas.getSize().x / somme_masse;

    // pastilles pour la légende des couleurs des joueurs
    ainet16::Position pos_pastille;
    pos_pastille.y = 50;
    sf::CircleShape pastille(10, 128);

    sf::Font police;
    police.loadFromFile("fonts/F-Zero GBA Text 1.ttf");

    // étiquettes pour chaque joueur
    sf::Text etiquette("Random text", police, 9);
    etiquette.setColor(sf::Color::Black);

    for (joueur=players.begin(); joueur!=players.end(); ++joueur)
    {
        window.setView(droite);
        // création de l'étiquette de chaque joueur
        pos_pastille.x = window_width*vue_carte.getViewport().width+ 10;
        pastille.setFillColor(colorFromPlayerId((*joueur).player_id, players.size()));
        pastille.setPosition(pos_pastille.x-5, pos_pastille.y-5);
        window.draw(pastille);

        etiquette.setPosition(pos_pastille.x + 2*pastille.getRadius(), pos_pastille.y);
        QString points = QString("%1").arg((*joueur).score);
        //QString numero = QString("%1").arg((*joueur).player_id);
        //std::string score = "Joueur " + numero.toStdString() + " : \n" + points.toStdString() + " points";
        std::string score = (*joueur).name + " : \n" + points.toStdString() + " points";
        etiquette.setString(score);
        window.draw(etiquette);

        pos_pastille.y += window_height/15;

        window.setView(bas);
        // création des rectangles qui représentent le score et la masse du joueur
        largeur_rect_score = (*joueur).score * facteur_score;
        largeur_rect_masse = (*joueur).mass * facteur_masse;

        rect_score.setSize(sf::Vector2f(largeur_rect_score, hauteur_rect));
        rect_score.setFillColor(colorFromPlayerId((*joueur).player_id, players.size()));
        rect_score.setPosition(sf::Vector2f(abscisse_rect_score,ordonnee_rect_score));
        abscisse_rect_score += largeur_rect_score; // on décale le rectangle pour afficher tous les rectangles côte à côte
        window.draw(rect_score);

        rect_masse.setSize(sf::Vector2f(largeur_rect_masse, hauteur_rect));
        rect_masse.setFillColor(colorFromPlayerId((*joueur).player_id, players.size()));
        rect_masse.setPosition(sf::Vector2f(abscisse_rect_masse,ordonnee_rect_masse));
        abscisse_rect_masse += largeur_rect_masse; // on décale le rectangle pour afficher tous les rectangles côte à côte
        window.draw(rect_masse);
    }

    // titre au-dessus de la barre des scores
    window.setView(bas);
    sf::Text texteScores("Repartition des scores :", police, 20);
    sf::Text texteMasses("Repartition des masses :", police, 20);

    texteScores.move(sf::Vector2f(0, ordonnee_rect_score - 0.7*hauteur_rect));
    texteScores.setColor(sf::Color::Black);
    window.draw(texteScores);

    texteMasses.move(sf::Vector2f(0, ordonnee_rect_masse - 0.7*hauteur_rect));
    texteMasses.setColor(sf::Color::Black);
    window.draw((texteMasses));
}

void Visu::afficheCadre()
{
    window.setView(vue_cadre);
    cadre.setFillColor(background_color);
    window.draw(cadre);
}

void Visu::afficheTour()
{
    std::string max_nb_turns_str = std::to_string(parameters.nb_turns);
    int nb_digits_max = max_nb_turns_str.size();

    std::ostringstream oss;
    oss << std::setw(nb_digits_max) << tourCourant;
    std::string str = oss.str(); // Contient le padding de nb_digits_max espaces

    window.setView(droite);
    sf::Text numero_tour("Tour " + str + "/" + max_nb_turns_str, police, 9);
    numero_tour.setColor(sf::Color::Black);
    numero_tour.setPosition(window_width*vue_carte.getViewport().width+ 10, 10);

    window.draw(numero_tour);

}

void Visu::afficheTout()
{
    window.clear(sf::Color::White);

    afficheCadre();
    afficheToutesCellules();
    afficheScore();
    afficheTour();

    window.display(); // dessine tous les objets avec lesquels on a appelé draw
}

void Visu::afficheFinPartie()
{
    //std::cout << "affichage fin de partie\n";
    window.clear(sf::Color::White);

    afficheCadre();
    afficheToutesCellules();
    afficheScore();
    afficheTour();

    // faire une zone de texte pour annoncer le gagnant
    window.setView(vue_cadre);

    //QString gagnant = QString("%1").arg(players[0].player_id);

    sf::Color couleur;
    if (background_color == sf::Color::White) {
        couleur = sf::Color::Black;
    }
    else {
        couleur = sf::Color::White;
    }

    sf::Text partie_terminee("Partie terminee", police, 20);
    float x = vue_carte.getViewport().width*window_width/2.0f*vue_carte.getViewport().width;
    //std::cout << x << std::endl;
    float y = vue_carte.getViewport().height*window_height/2.0f*vue_carte.getViewport().height;
    //std::cout << y << std::endl;
    partie_terminee.setColor(couleur);
    sf::FloatRect textRect = partie_terminee.getLocalBounds();
    partie_terminee.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    partie_terminee.setPosition(sf::Vector2f(x/2, y/6));
    window.draw(partie_terminee);

    sf::Text le_gagnant_est("Le gagnant est :", police, 20);
    le_gagnant_est.setColor(couleur);
    textRect = le_gagnant_est.getLocalBounds();
    le_gagnant_est.setOrigin(textRect.left + textRect.width/2.0, textRect.top + textRect.height/2.0);
    le_gagnant_est.setPosition(sf::Vector2f(x/2, y/2));
    window.draw(le_gagnant_est);

    sf::Text joueur(players[0].name, police, 20);
    joueur.setColor(couleur);
    textRect = joueur.getLocalBounds();
    joueur.setOrigin(textRect.left + textRect.width/2.0, textRect.top + textRect.height/2.0);
    joueur.setPosition(sf::Vector2f(x/2, y/2+y/6));
    window.draw(joueur);

    window.display();
}

void Visu::handleEvents()
{
    sf::Event event;
    while (window.pollEvent(event)) {
        switch(event.type) {

        // fermeture de la fenêtre
        case sf::Event::Closed:
            window.close();
            break;

            // clic de souris
        case sf::Event::MouseButtonPressed:
            switch(event.mouseButton.button) {

            case sf::Mouse::Left:
                centreVueSouris();
                break;

            default:
                break;
            }

            // roulette de souris
        case sf::Event::MouseWheelMoved:

            if (event.mouseWheel.delta > 0) {
                zoom();
            }

            if (event.mouseWheel.delta < 0) {
                dezoom();
            }

            // appui sur un bouton du clavier
        case sf::Event::KeyPressed:
            //qDebug() << "une touche a été pressée\n";
            switch(event.key.code) {

            case sf::Keyboard::I:
                inverseCouleurs();
                break;

            case sf::Keyboard::N:
                toggleNeutralCells();
                break;

            case sf::Keyboard::Add:
                zoom();
                break;

            case sf::Keyboard::Subtract:
                dezoom();
                break;

            case sf::Keyboard::Equal:
                resetCarte();
                break;

            case sf::Keyboard::Right:
                deplaceVueDroite();
                break;

            case sf::Keyboard::Left:
                deplaceVueGauche();
                break;

            case sf::Keyboard::Up:
                deplaceVueHaut();
                break;

            case sf::Keyboard::Down:
                deplaceVueBas();
                break;

            default:
                break;
            }

        default:
            break;

        }
    }

}

void Visu::onGameEnd(int winnerPlayerId, std::vector<ainet16::GameEndsPlayer> endPlayers)
{
    partieEnCours = false;
    int a=winnerPlayerId;
    a++;

    std::cout << "entrée dans onGameEnd\n";

    // mettre à jour les scores pour l'affichage du classement
    std::sort(players.begin(), players.end(), CompareIdJoueurs());
    for (uint i=0; i<players.size(); ++i) {
        players[i].score = endPlayers[i].score;
    }
    //std::cout << "les scores sont mis à jour\n";
    std::sort(players.begin(), players.end(), CompareScoresJoueurs());
}

void Visu::onException()
{
    partieEnCours = false;
    std::sort(players.begin(), players.end(), CompareScoresJoueurs());
}

void Visu::zoom()
{
    vue_carte.zoom(0.75);
}

void Visu::dezoom()
{
    vue_carte.zoom(1/0.75);
}

void Visu::resetCarte()
{
    vue_carte.reset(sf::FloatRect(0, 0, parameters.map_width, parameters.map_height));
}

void Visu::toggleNeutralCells()
{
    afficheCellulesNeutres = !afficheCellulesNeutres;
}

void Visu::deplaceVueDroite()
{
    vue_carte.move(5, 0);
}

void Visu::deplaceVueGauche()
{
    vue_carte.move(-5, 0);
}

void Visu::deplaceVueHaut()
{
    vue_carte.move(0, -5);
}

void Visu::deplaceVueBas()
{
    vue_carte.move(0, 5);
}

void Visu::centreVueSouris()
{
    std::cout << "centre vue souris\n";

    // coordonnées de la souris dans la fenêtre
    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);

    // conversion dans la carte
    sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos, vue_carte);

    vue_carte.setCenter(worldPos);
}

void Visu::addNewCell(Cellule *cellule)
{
    allCells[cellule->id()] = cellule;
    allCellsByMass.push_back(cellule);
    //sort(allCellsByMass.begin(), allCellsByMass.end(), CompareMasseCellules());
}

void Visu::removeCell(quint32 id)
{
    // Let the cell be removed from the allCellsByMass vector
    auto it = std::find_if(allCellsByMass.begin(), allCellsByMass.end(),
                        [id](const Cellule * cell) -> bool
                        {
                            return cell->id() == id;
                        });

    Q_ASSERT(it != allCellsByMass.end());
    allCellsByMass.erase(it);

    // Let the cell be removed from the allCells map
    allCells.erase(id);
}

void Visu::addNewPlayer(ainet16::TurnPlayer p)
{
    players.push_back(p);
    /*std::sort(players.begin(), players.end(), CompareIdJoueurs());
    for (int i=0; i<players.size(); ++i) {
        std::cout << players[i].id << std::endl;
    }*/
}

void Visu::inverseCouleurs()
{
    if (background_color == sf::Color::White) {
        background_color = sf::Color::Black;
        borders_color = sf::Color::White;
    }
    else {
        background_color = sf::Color::White;
        borders_color = sf::Color::Black;
    }
}

int Visu::nbeJoueurs()
{
    return players.size();
}

bool Visu::enCours()
{
    return partieEnCours;
}

sf::Color colorFromPlayerId(quint32 playerId, int nbePlayers)
{
    double saturation = 1;
    double value = 1;
    double hue = (double(playerId)/double(nbePlayers) * 360);

    double r, g, b;
    hsvToRgb(hue, saturation, value, r, g, b);

    return sf::Color(sf::Uint8(255*r), sf::Uint8(g*255), sf::Uint8(b*255));
}

void hsvToRgb(double h, double s, double v, double & r, double & g, double & b) {
    // r g b, s v entre [0,1], h entre [0,360]
    if (s == 0) // Achromatic (grey)
    {
        r = g = b = v;
        return;
    }

    h /= 60;            // sector 0 to 5
    int i = floor(h);
    float f = h-i;      // factorial part of h
    float p = v*(1-s);
    float q = v*(1-s*f);
    float t = v*(1-s*(1-f));

    switch(i)
    {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    default:    // case 5:
        r = v;
        g = p;
        b = q;
        break;
    }
}
