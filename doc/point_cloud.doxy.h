/**
 * @~english
 * @taomoduledescription{PointCloud, Point clouds}
 *
 * <tt>import PointCloud</tt> - Creates and displays point clouds.
 * A cloud is identified by its name.
 * Point data may be read from a file, or added dynamically. A point cloud
 * may be monochrome (all points having the same color attributes: the
 * current ones) or each point can have its own color. The point size can
 * be defined for each cloud.
 *
 * Here is an example:
 * @code
import PointCloud

Created -> 0
if Created = 0 then
    cloud_only ""
    for i in -5..5 loop
        for j in -5..5 loop
            for k in -5..5 loop
                cloud_add "Cube", i * 20, j * 20, k * 20
    cloud_point_size "Cube", 3
    Created := 1

page "Cube",
    clear_color 0, 0, 0, 1
    rotatey 6 * page_time
    rotatex 5 * page_time
    color "white"
    cloud "Cube"
 * @endcode
 * @image html Cube.png "Point cloud example"
 *
 * @endtaomoduledescription{PointCloud}
 *
 * @~french
 * @taomoduledescription{PointCloud, Nuages de points}
 *
 * <tt>import PointCloud</tt> - Permet de créer et d'afficher des nuages de points.
 * Un nuage est identifié
 * par son nom. Les points peuvent être lus depuis un fichier ou ajoutés
 * dynamiquement. Un nuage peut être monochrome, auquel cas tous les points qui
 * le composent ont la couleur courante (cf. @p color). Ou bien chaque point
 * peut avoir sa propre couleur. La taille des points peut être définie pour
 * chaque nuage.
 *
 * Voici un exemple:
 * @code
import PointCloud

Créé -> 0
if Créé = 0 then
    cloud_only ""
    for i in -5..5 loop
        for j in -5..5 loop
            for k in -5..5 loop
                cloud_add "Cube", i * 20, j * 20, k * 20
    cloud_point_size "Cube", 3
    Créé := 1

page "Cube",
    clear_color 0, 0, 0, 1
    rotatey 6 * page_time
    rotatex 5 * page_time
    color "white"
    cloud "Cube"
 * @endcode
 * @image html Cube.png "Point cloud example"
 *
 * @endtaomoduledescription{PointCloud}
 * @~
 * @{
 */

/**
 * @~english
 * Deletes a point cloud.
 * @~french
 * Détruit un nuage de points.
 * @~
 * @see cloud_only.
 */
cloud_drop(name:text);

/**
 * @~english
 * Keeps only one cloud.
 * Deletes all point clouds except the one specified. If @p name is an
 * empty string, all clouds are destroyed.
 * @~french
 * Conserve un seul nuage de points.
 * Tous les nuages sont détruits, sauf celui dont le nom est précisé. Si
 * @p name est une chaîne vide, tous les nuages de points sont détruits.
 * @~
 * @see cloud_drop.
 */
cloud_only(name:text);

/**
 * @~english
 * Fills a cloud with random points.
 * If no cloud called @p name exists when this function is called, a new
 * cloud is created and @p n points are added. Point coordinates are random
 * values in the range [0.0, 1.0]. @n
 * If the cloud already exists, points are either removed or added so that
 * the cloud has exactly @p n points on return.
 * @~french
 * Remplit un nuage avec des points aléatoires.
 * Si le nuage @p name n'existe pas, il est créé et @p n points lui sont
 * ajoutés. Les coordonnées des points sont aléatoires et sont comprises entre
 * 0.0 et 1.0. @n
 * Si le nuage existe déjà, des points sont supprimés ou ajoutés de sorte que
 * le nuage contienne exactement @p n points.
 */
cloud_random(name:text, n:integer);

/**
 * @~english
 * Fills a cloud with random colored points.
 * Similar to @ref cloud_random, except that instead of taking the current
 * color, points are individually assigned a random color. That is, the red,
 * green, blue and alpha color components take a random value between 0.0
 * and 1.0. @n
 * If the cloud already exists and does not contain colored points, this
 * function behaves like @ref cloud_random.
 * @~french
 * Remplit un nuage avec des points aléatoires colorés.
 * Similaire à @ref cloud_random, sauf qu'au lieu de prendre la couleur
 * courante, chaque point a une couleur aléatoire. Les composantes rouge,
 * verte, bleue et alpha prennent une valeur aléatoire comprise entre 0.0
 * et 1.0. @n
 * Si le nuage existe déjà et ne contient pas de points colorés, cette fonction
 * se comporte comme @ref cloud_random.
 */
cloud_random_colored(name:text, n:integer);

/**
 * @~english
 * Adds a point to a cloud.
 * The cloud is created if it does not exist.
 * @~french
 * Ajoute un point à un nuage de points.
 * Le nuage est créé s'il n'existe pas.
 */
cloud_add(name:text, x:real, y:real, z:real);

/**
 * @~english
 * Adds a colored point to a cloud.
 * The cloud is created if it does not exist. If the cloud exists but does not
 * contain colored points, the color components are ignored. @n
 * @p r, @p g @p b
 * and @p a are the red, green, blue and alpha values, respectively. They
 * should be comprised between 0.0 and 1.0. @n
 * It is not possible to add points to an optimized cloud.
 * In this case, an error is reported. See @ref cloud_optimize.
 * @~french
 * Ajoute un point coloré à un nuage de points.
 * Le nuage est créé s'il n'existe pas. S'il existe, mais ne contient pas des
 * points colorés, les composantes couleur sont ignorées. @n
 * @p r, @p g @p b
 * et @p a représentent respectivement les canaux rouge, vert, bleu et alpha.
 * Leur valeur doit être comprise entre 0.0 et 1.0. @n
 * Il n'est pas possible d'ajouter un point à un nuage qui a été optimisé, cf.
 * @ref cloud_optimize.
 */
cloud_add(name:text, x:real, y:real, z:real, r:real, g:real, b:real, a:real);

/**
 * @~english
 * Creates a point cloud from a data file in text format.
 * The cloud is created if it does not exist. If it exists, all points are
 * first discarded. @n
 * Each line of the source file is parsed as a list of real numbers, separated
 * by a given character, @p sep. To read a CSV file, use <tt>sep = ","</tt>
 * (comma). @n
 * @p xi, @p yi, @p zi are the indices where to find the x, y and z values,
 * respectively. For instance if the file is in the format <tt>x,y,z</tt> you
 * would pass <tt>xi = 1</tt>, <tt>yi = 2</tt> and <tt>zi = 3</tt>. If however
 * the format is <tt>z,y,x</tt> you would pass <tt>xi = 3</tt>,
 * <tt>yi = 2</tt> and <tt>zi = 1</tt>. @n
 * File load occurs in the background. Use @ref cloud_loaded to know when
 * load is complete.@n
 * If the file changes after being loaded, it is reloaded automatically.
 * @~french
 * Crée un nuage de points à partir d'un fichier de valeurs numériques.
 * Le nuage est créé s'il n'existe pas. Mais s'il existe, les points qu'il
 * contient sont effacés avant que les nouveaux points soient ajoutés. @n
 * Chaque ligne du fichier source est interprétée comme une liste de nombres
 * réels, séparés par un caractère donné, @p sep. Pour lire un fichier CSV,
 * il suffit de passer <tt>sep = ","</tt> (virgule). @n
 * @p xi, @p yi, @p zi sont les indices qui définissent la position des valeurs
 * x, y et z, respectivement. Par exemple, si le fichier est au format
 * <tt>x,y,z</tt>, il convient de passer <tt>xi = 1</tt>, <tt>yi = 2</tt> et
 * <tt>zi = 3</tt>. Si par contre le fichier contient <tt>z,y,x</tt>, les
 * valeurs correctes sont <tt>xi = 3</tt>, <tt>yi = 2</tt> et <tt>zi = 1</tt>.
 * @n
 * Le chargement s'effectue en tâche de fond. Utilisez @ref cloud_loaded pour
 * savoir si le chargement est terminé.@n
 * Si le fichier est modifié après avoir été chargé, il est rechargé
 * automatiquement.
 * @~
 * @see cloud_loaded
 */
cloud_load_data(name:text, file:text, sep:text, xi:integer, yi:integer, zi:integer);

/**
 * @~english
 * Creates a point cloud from a data file in text format.
 * This function is similar to the other form of cloud_data, but it allows to
 * read color information for each point. @n
 * @p ri, @p gi, @p bi and @p ai are normally the indices of the red, green,
 * blue and alpha components, respectively. However, negative values are
 * interpreted as constant color values. For instance, suppose you have a
 * file in <tt>x,y,z,r,g,b</tt> format with no alpha channel. To force alpha
 * to 1.0, you would pass
 * <tt>xi=1 yi=2 zi=3 ri=4 gi=5 bi=6 ai=-1.0</tt>. @n
 * @p scale is a scaling factor applied to any color value
 * read from the file. Scaling is not applied to constant values (passed as
 * negative numbers). The typical use case for @p scale is to adapt color
 * values in the 0-255 range into the expected 0.0-1.0 range: simply set
 * <tt>scale = 1.0/255</tt>.
 * @~french
 * Crée un nuage de points à partir d'un fichier de valeurs numériques.
 * Cette fonction est similaire à l'autre forme de cloud_data, sauf qu'elle
 * permet de lire des informations de couleur pour chaque point. @n
 * @p ri, @p gi, @p bi et @p ai sont normalement les indices des composantes
 * rouge, verte, bleue et alpha, respectivement. Cependant, toute valeur
 * négative est interprétée comme une valeur constante. Par exemple, supposons
 * que le fichier soit au format <tt>x,y,z,r,g,b</tt> sans canal alpha. Pour
 * forcer alpha à 1.0, il faut passer
 * <tt>xi=1 yi=2 zi=3 ri=4 gi=5 bi=6 ai=-1.0</tt>. @n
 * @p scale est un facteur multiplicatif qui s'applique à toute valeur de
 * couleur lue
 * depuis le fichier. Ce facteur ne s'applique pas aux valeurs constantes
 * (passées sous forme de nombres négatifs). L'utilisation typique de @p scale
 * est d'adapter des couleurs entre 0 et 255 à l'échelle attendue, entre 0.0 à
 * 1.0. Pour cela, utiliser <tt>scale = 1.0/255</tt>.
 * @~
 * @see cloud_loaded
 */
cloud_load_data(name:text, file:text, sep:text, xi:integer, yi:integer, zi:integer,
                scale:real, ri:real, gi:real, bi:real, ai:real);

/**
 * @~english
 * Returns progress information about cloud_load_data.
 * Returns a value between 0.0 and 1.0 (that is, between 0 and 100%). If no
 * load has been requested, the function returns 0.0.
 * @~french
 * Renvoie la progression de cloud_load_data.
 * La valeur de retour est comprise entre 0.0 et 1.0 (autrement dit, entre
 * 0 et 100%). Si aucun chargement de donnée n'a été demandé, la valeur de
 * retour est 0.0.
 */
cloud_loaded(name:text);

/**
 * @~english
 * Attempts to reduce memory usage of a cloud.
 * The point cloud implementation automatically takes advantage of OpenGL
 * Vertex Buffer Objects (VBOs) when available. Point data are transfered
 * into the graphic card's memory once, and can later be drawn as
 * many times as necessary without any additional data transfer between the
 * main memory and the GPU. This can dramatically improve performance,
 * especially with large data sets. However, point data are normally kept
 * in main memory to allow for new points to be added to the cloud
 * (@ref cloud_add). @n
 * This function will free the point data from the main memory and keep only
 * the data in the Vertex Buffer Objects. @n
 * This function does nothing if:
 * - VBOs are not supported, or
 * - the point cloud is currently being loaded from a file, or
 * - some points have been added with @ref cloud_add. They cannot be deleted
 * otherwise they would be lost when the OpenGL context changes (switching
 * to/from quad buffer stereoscopic mode). @n
 * If a cloud has been optimized and the GL context changes, the cloud is
 * re-created in the new context (that is, @ref cloud_random or
 * @ref cloud_random_colored or @ref cloud_load_data is executed again).
 * @~french
 * Essaie de réduire l'utilisation mémoire d'un nuage de points.
 * L'implémentation des nuages de points tire parti de la fonctionalité
 * OpenGL : Vertex Buffer Objects (VBOs), lorsqu'elle est disponible. Les
 * données des points sont transférées dans la mémoire de la carte graphique
 * une fois, et sont ensuite utilisées autant de fois que nécessaire pour le
 * tracé, sans nécessiter de transfert supplémentaire entre la mémoire
 * principale et la carte. Ceci permet d'améliorer significativement les
 * performances, en particulier lorsque les points sont très nombreux.
 * Cependant, les points sont tout de même conservés dans la mémoire
 * principale pour permettre l'ajout de nouveaux points (@ref cloud_add). @n
 * Cette fonction détruit les données qui sont en mémoire pour ne conserver que
 * celles qui sont dans les VBOs. @n
 * Cette fonction en fait rien si :
 * - la carte graphique ne permet pas d'utiliser les VBOs, ou si
 * - le nuage est en cours de chargement (@ref cloud_load_data), ou si
 * - des points ont été ajoutés par @ref cloud_add. En effet, ces points
 * seraient perdus lors d'un changement de contexte OpenGL (passage en mode
 * quad buffer par exemple). @n
 * Si un nuage a été optimisé, et le context GL change, alors le nuage est
 * recréé. C'est à dire, @ref cloud_random ou @ref cloud_random_colored ou
 * @ref cloud_load_data est exécuté de nouveau).
 */
cloud_optimize(name:text);

/**
 * @~english
 * Sets the size of the points for a given cloud.
 * The default point size is 1.0.
 * @~french
 * Permet de choisir la taille des points d'un nuage.
 * La taille par défaut des points est 1.0.
 */
cloud_point_size(name:text, s:real);

/**
 * @~english
 * Displays a point cloud.
 * @~french
 * Affiche un nuage de points.
 */
cloud(name:text);

/**
 * @~english
 * Enables or disables point sprites mode.
 * Use this mode to show an image at the location of each point. When this
 * mode is active, the following OpenGL calls are made before the cloud is
 * drawn:
 * @~french
 * Permet d'activer le mode sprites.
 * Utilisez ce mode pour afficher une image à l'emplacement de chaque point.
 * Lorsque le mode sprites est actif, les appels suivants sont effectués
 * avant le tracé du nuage de point :
 * @~
 * @code
glEnable(GL_POINT_SPRITE);
glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
 * @endcode
 * @since 1.013
 */
cloud_point_sprites(name:text, on:boolean);


/**
 * @~english
 * Enables or disables programmable point size.
 * Use this mode to enable a shader to set the point size using gl_PointSize.
 * When this mode is active, the following code is activated before
 * drawing points:
 * @~french
 * Permet d'activer le mode programmable pour les tailles de points.
 * Utilisez ce mode pour pouvoir changer la taille des points de façon
 * dynamique à l'intérieur d'un programme de shader. Dans ce mode, le code
 * suivant est activé avant le tracé des points:
 * @~
 * @code
glEnable(GL_PROGRAM_POINT_SIZE);
 * @endcode
 * @since 1.016
 */
cloud_point_sprites(name:text, on:boolean);

/**
 * @}
 */
