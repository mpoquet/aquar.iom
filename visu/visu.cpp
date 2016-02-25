#include "visu.hpp"

Visu::Visu()
{
    // initialisation des attributs : pour la plupart il faut attendre un Welcome pour avoir les données
    window_height = 768;
    window_width = 1024;
    background_color = sf::Color::Black;
    borders_color = sf::Color::White;

    cadre.setFillColor(background_color);
    cadre.setOutlineColor(borders_color);
    cadre.setOutlineThickness(1);

    // création de la fenêtre
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    window.create(sf::VideoMode(window_width, window_height), "Le jeu des cellules qui se mangent", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(false);
    window.setKeyRepeatEnabled(true); // activer/désactiver la répétition des touches si on les maintient appuyées

    droite.reset(sf::FloatRect(0.85*window_width, 0, 0.15*window_width, window_height));
    droite.setViewport(sf::FloatRect(0.85, 0, 0.15, 1));

    bas.reset(sf::FloatRect(0, 0.85*window_height+1, window_width, 0.15*window_height));
    bas.setViewport(sf::FloatRect(0, 0.85, 1, 0.15));

}

Visu::~Visu()
{
}

void Visu::onWelcomeReceived(const ainet16::Welcome &welcome)
{
    /// enregistrer les paramètres de la partie dans la classe
    parameters.map_width = welcome.parameters.map_width;
    parameters.map_height = welcome.parameters.map_height;
    parameters.min_nb_players = welcome.parameters.min_nb_players;
    parameters.max_nb_players = welcome.parameters.max_nb_players;
    parameters.mass_absorption = welcome.parameters.mass_absorption;
    parameters.minimum_mass_ratio_to_absorb = welcome.parameters.minimum_mass_ratio_to_absorb;
    parameters.minimum_pcell_mass = welcome.parameters.minimum_pcell_mass;
    parameters.radius_factor = welcome.parameters.radius_factor;
    parameters.max_cells_per_player = welcome.parameters.max_cells_per_player;
    parameters.mass_loss_per_frame = welcome.parameters.mass_loss_per_frame; // not useful for visu
    parameters.base_cell_speed = welcome.parameters.base_cell_speed; // not useful for visu
    parameters.speed_loss_factor = welcome.parameters.speed_loss_factor; // not useful for visu
    parameters.virus_mass = welcome.parameters.virus_mass; // tous les virus ont la même masse
    parameters.virus_creation_mass_loss = welcome.parameters.virus_creation_mass_loss; // not useful for visu
    parameters.virus_max_split = welcome.parameters.virus_max_split; // not useful for visu
    parameters.nb_starting_cells_per_player = welcome.parameters.nb_starting_cells_per_player; // not useful for visu
    parameters.player_cells_starting_mass = welcome.parameters.player_cells_starting_mass; // not useful for visu
    parameters.initial_neutral_cells_mass = welcome.parameters.initial_neutral_cells_mass;
    parameters.initial_neutral_cells_repop_time = welcome.parameters.initial_neutral_cells_repop_time;

    /// initialiser la vue
    vue_carte.reset(sf::FloatRect(0, 0, parameters.map_width, parameters.map_height));
    vue_carte.setViewport(sf::FloatRect(0, 0, 0.85, 0.85));

    /*cadre.setSize(sf::Vector2f(parameters.map_width, parameters.map_height));
    vue_cadre.reset(sf::FloatRect(0, 0, parameters.map_width/0.85, parameters.map_height/0.85));*/
    cadre.setSize(sf::Vector2f(vue_carte.getViewport().width, vue_carte.getViewport().height));
    vue_cadre.reset(sf::FloatRect(0, 0, cadre.getSize().x, cadre.getSize().y));

    /// initialiser l'ensemble des cellules avec leurs positions initiales
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

    // pas besoin d'initialiser les joueurs : on va les recevoir dans un Turn


}

void Visu::onTurnReceived(const ainet16::Turn &turn)
{
    std::cout << "on a reçu un nouveau tour\n";
    if (players.size()==0) {
        /// C'est la première fois qu'on reçoit un Turn : initialisation de la liste des joueurs
        std::cout << "Création des joueurs\n";
        for (unsigned int i=0; i<turn.players.size(); ++i) {
            addNewPlayer(turn.players[i]);
        }
    }

    else {
        /// mettre à jour le score des joueurs
        std::vector<ainet16::TurnPlayer> temp_Players;
        for (uint i=0; i<players.size(); ++i) {
            temp_Players.push_back(turn.players[i]);
        }
        std::sort(temp_Players.begin(), temp_Players.end(), CompareIdJoueurs()); // génère erreurs de compilation quand on utilise directement turn.players
        std::sort(players.begin(), players.end(), CompareIdJoueurs());

        for (uint i=0; i<players.size(); ++i) {
            players[i].mass = turn.players[i].mass;
            players[i].nb_cells = turn.players[i].nb_cells;
            players[i].score = turn.players[i].score;
        }
    }
    /// mettre à jour les cellules
    // toutes les cellules sont mortes. Les mettre à jour va les rendre vivantes. Celles qui seront encore mortes à la fin de la
    // fonction seront donc des cellules supprimées
    std::map<quint32, Cellule*>::iterator it;
    for (it=allCells.begin(); it!=allCells.end(); ++it) {
        (*it).second->estVivante = false;
    }

    std::cout << "màj des virus\n";
    for (unsigned int i=0; i<turn.viruses.size(); ++i) {
        int indice = turn.viruses[i].id;
        // vérifier si allCells[indice] existe déjà
        if (allCells.count(indice) == 0) {
            // créer la cellule et l'ajouter dans l'ensemble
            std::cout << "Création du virus " << indice << std::endl;
            Cellule* cell = new Cellule(turn.viruses[i], parameters.virus_mass);
            addNewCell(cell);
        }
        else {
            allCells[indice]->position.x = turn.viruses[i].position.x;
            allCells[indice]->position.y = turn.viruses[i].position.y;
            allCells[indice]->estVivante = true;
        }
    }

    std::cout << "màj des cellules des joueurs\n";
    for (unsigned int i=0; i<turn.pcells.size(); ++i) {
        int indice = turn.pcells[i].pcell_id;
        // vérifier si allCells[indice] existe déjà
        if (allCells.count(indice) == 0) {
            // créer la cellule et l'ajouter dans l'ensemble
            std::cout << "Création de la cellule joueuse " << indice << std::endl;
            Cellule* cell = new Cellule(turn.pcells[i], players.size());
            addNewCell(cell);
        }
        else {
            allCells[indice]->position.x = turn.pcells[i].position.x;
            allCells[indice]->position.y = turn.pcells[i].position.y;
            allCells[indice]->mass = turn.pcells[i].mass;
            allCells[indice]->remaining_isolated_turns = turn.pcells[i].remaining_isolated_turns;
            allCells[indice]->estVivante = true;
        }
    }
    std::cout << "màj des cellules neutres initiales\n";
    // On sait que les cellules initiales neutres ont les numéros de 0 à nbCellulesInitiales - 1
    int indice(0);
    for (unsigned int i=0; i<turn.initial_ncells.size(); ++i) {
        // Les cellules initiales neutres ne se déplacent pas
        allCells[indice]->remaining_turns_before_apparition = turn.initial_ncells[i].remaining_turns_before_apparition;
        allCells[indice]->estVivante = true;
        ++indice;
    }
    std::cout << "màj des cellules neutres non initiales\n";
    for (unsigned int i=0; i<turn.non_initial_ncells.size(); ++i) {
        int indice = turn.non_initial_ncells[i].ncell_id;
        // vérifier si allCells[indice] existe déjà
        if (allCells.count(indice) == 0) {
            // créer la cellule et l'ajouter dans l'ensemble
            std::cout << "Création de la cellule neutre" << indice << std::endl;
            Cellule* cell = new Cellule(turn.non_initial_ncells[i]);
            addNewCell(cell);
        }
        else {
            //std::cout << "Mise à jour de la cellule neutre" << indice << std::endl;
            allCells[indice]->position.x = turn.non_initial_ncells[i].position.x;
            allCells[indice]->position.y = turn.non_initial_ncells[i].position.y;
            allCells[indice]->mass = turn.non_initial_ncells[i].mass;
            allCells[indice]->estVivante = true;
        }
    }
}

void Visu::afficheCellule(Cellule* cellule)
{
    if ((cellule->remaining_turns_before_apparition != 0)) {
        return; // La cellule n'est pas encore apparue donc on ne l'affiche pas
    }

    if (cellule->estVivante == false) {
        qDebug() << "Cellule morte\n";
        // la cellule est morte : on ne l'affiche pas et on la supprime de l'ensemble des cellules
        removeCell(cellule->id());
        return;
    }

    qDebug() << cellule->id();

    float rayon = cellule->mass * parameters.radius_factor;

    sf::CircleShape cercle(rayon, 128);
    cercle.setFillColor(cellule->color);

    // mettre un contour pour les cellules isolées, les cellules neutres et les virus
    if ((cellule->remaining_isolated_turns != 0) | (cellule->typeDeCellule == initialNeutral) | (cellule->typeDeCellule == nonInitialNeutral) | (cellule->typeDeCellule == virus)) {
        cercle.setOutlineColor(borders_color);
        cercle.setOutlineThickness(rayon/6);
        rayon = 5.0/6.0 * rayon; // on change la valeur du rayon car la bordure est rajoutée à l'extérieur du circleshape
        cercle.setRadius(rayon);
    }
    cercle.setPosition(cellule->position.x - rayon, cellule->position.y - rayon); // setPosition prend la position du coin supérieur gauche =(1
    window.draw(cercle);

}

void Visu::afficheToutesCellules() {
    window.setView(vue_carte);

    std::vector<Cellule*>::iterator it;
    for (it=allCellsByMass.begin() ; it!=allCellsByMass.end() ; ++it) {
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

    // trier les joueurs par score décroissant
    sort(players.begin(), players.end(), CompareScoresJoueurs());

    // rectangles pour la représentation du score relatif des joueurs
    sf::RectangleShape rect; // rectangle de dimensions (0,0)
    float hauteur_rect(1.0/2.0 * ((bas.getSize().y))), largeur_rect(0); // la moitié de la distance qui reste entre le bas du plateau et le bas de la fenêtre
    //std::cout << window_height*0.15 << " " << bas.getSize().y << " " << 0.85*window_height << " " << vue_carte.getViewport().height << std::endl;
    float abscisse_rect(0), ordonnee_rect(vue_carte.getViewport().height*window_height + hauteur_rect);

    std::vector<ainet16::TurnPlayer>::iterator joueur;
    float somme_scores(0);
    for (joueur=players.begin(); joueur!=players.end(); ++joueur) {
        somme_scores += (*joueur).score;
    }
    float facteur_score = bas.getSize().x / somme_scores;

    // pastilles pour la légende des couleurs des joueurs
    ainet16::Position pos_pastille;
    pos_pastille.y = 20;
    sf::CircleShape pastille(10, 128);

    sf::Font police;
    police.loadFromFile("fonts/F-Zero GBA Text 1.ttf");

    // étiquettes pour chaque joueur
    sf::Text etiquette("Random text", police, 9);
    etiquette.setColor(sf::Color::Black);

    for (joueur=players.begin(); joueur!=players.end(); ++joueur) {
        window.setView(droite);
        // création de l'étiquette de chaque joueur
        pos_pastille.x = window_width*vue_carte.getViewport().width+ 10;
        pastille.setFillColor(colorFromPlayerId((*joueur).player_id, players.size()));
        pastille.setPosition(pos_pastille.x-5, pos_pastille.y-5);
        window.draw(pastille);

        etiquette.setPosition(pos_pastille.x + 2*pastille.getRadius(), pos_pastille.y);
        QString points = QString("%1").arg((*joueur).score);
        QString numero = QString("%1").arg((*joueur).player_id);
        std::string score = "Joueur " + numero.toStdString() + " : \n" + points.toStdString() + " points";
        etiquette.setString(score);
        window.draw(etiquette);

        pos_pastille.y += window_height/15;

        window.setView(bas);
        // création du rectangle qui représente le score du joueur
        largeur_rect = (*joueur).score * facteur_score;
        rect.setSize(sf::Vector2f(largeur_rect, hauteur_rect));
        rect.setFillColor(colorFromPlayerId((*joueur).player_id, players.size()));
        rect.setPosition(sf::Vector2f(abscisse_rect,ordonnee_rect));
        abscisse_rect += largeur_rect; // on décale le rectangle pour afficher tous les rectangles côte à côte
        window.draw(rect);
    }

    // titre au-dessus de la barre des scores
    window.setView(bas);
    sf::Text texteScores("Repartition des scores :", police, 25);
    //texteScores.move(sf::Vector2f(0, vue_cadre.getViewport().height*window_height+hauteur_rect));
    texteScores.move(sf::Vector2f(0, ordonnee_rect - 0.7*hauteur_rect));
    texteScores.setColor(sf::Color::Black);
    window.draw(texteScores);
}

void Visu::afficheCadre()
{
    window.setView(vue_cadre);
    cadre.setFillColor(background_color);
    window.draw(cadre);
}

void Visu::afficheTout()
{
    window.clear(sf::Color::White);

    afficheCadre();
    afficheToutesCellules();
    afficheScore();

    window.display(); // dessine tous les objets avec lesquels on a appelé draw
}

void Visu::handleEvents(ainet16::Turn & tour)
{
    sf::Event event;
    window.pollEvent(event);
    switch(event.type) {

    // fermeture de la fenêtre
    case sf::Event::Closed:
        window.close();
        break;

        // appui sur un bouton du clavier
    case sf::Event::KeyPressed:
        switch(event.key.code) {

        case sf::Keyboard::I:
            // todo : inverser la couleur de fond de la fenêtre et celle des contours (cellules + cadre)
            inverseCouleurs();
            break;

        case sf::Keyboard::T:
            // test de la fonction onTurnReceived
            tour.viruses[0].position.y += 100;
            tour.pcells[0].position.x += 100;
            tour.pcells[2].position.y -= 20;
            tour.initial_ncells[0].remaining_turns_before_apparition = 0;
            tour.non_initial_ncells[0].mass = 50;
            onTurnReceived(tour);
            tour.players[2].score = 21;
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

void Visu::addNewCell(Cellule *cellule)
{
    allCells[cellule->id()] = cellule;
    allCellsByMass.push_back(cellule);
    sort(allCellsByMass.begin(), allCellsByMass.end(), CompareMasseCellules());
}

void Visu::removeCell(quint32 id)
{
    qDebug() << "entrée dans removeCell\n";

    // retirer la cellule de allCellsByMass
    bool ok = false;
    uint i(0);
    while ((i<allCellsByMass.size()) | (ok==false)) {
        qDebug() << i << endl;
        if (allCellsByMass[i]->id() == id) {
            ok = true;
            allCellsByMass.erase(allCellsByMass.begin()+i);
        }
        ++i;
    }

    qDebug() << "supprimer la cellule de allCells\n";
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
