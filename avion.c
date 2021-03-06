
#include "avion.h"

// caractéristiques du déplacement de l'avion
struct deplacement dep;

// coordonnées spatiales de l'avion
struct coordonnees coord;

// numéro de vol de l'avion : code sur 5 caractères
char numero_vol[6];

/**
 * Gestion du multicast
 */
#define PORTMULTI 12345
#define MULTICASTGROUP "225.0.0.37"


/**
 * Gestion connexion TCP
 */
struct sockaddr_in addr_avion; /* client */
struct sockaddr_in addr_SGCA; /* server */
struct hosten *host_SGCA; /* identifiant de la machine où se trouve le server */

int sock;

/********************************
 ***  3 fonctions à implémenter
 ********************************/
int ouvrir_communication_TCP() {
   int value = 1;
   //Create socket
   if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror("Could not create socket");
      exit(EXIT_FAILURE);
   }
   puts("Socket created");

   //Connect to remote server
   if (connect(sock , (struct sockaddr *) &addr_SGCA , sizeof(addr_SGCA)) < 0) {
      perror("connect failed. Error");
      return 1;
   }

   puts("Connected\n");

   /**
    * Envoie les caractéristiques à l'SGCA
    */
   while(1) {
      envoyer_caracteristiques();
      return 0;
   }
}

/**
 * Crée la socket d'écoute du serveur multicast SGCA.
 */
int ouvrir_communication() {
   int nbytes,addrlen;
   struct ip_mreq mreq;
   struct sockaddr_in addr_envoyeur; /* server */

   u_int reuse=1;            /*** MODIFICATION TO ORIGINAL */
   int a=0;
   int port;
   char *endptr;

   port = PORTMULTI;

   /* create what looks like an ordinary UDP socket */
   if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP /* ou 0 */)) < 0) {
      perror("socket");
      exit(EXIT_FAILURE);
   }
  //else printf("Opening datagram socket....OK.\n");

   /**
    * Enable SO_REUSEADDR to allow multiple instances of this
    * allow multiple sockets to use the same PORT number
    */
   if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) < 0) {
      perror("Reusing ADDR failed");
      close(sock);
      exit(EXIT_FAILURE);
   }
   //else printf("Setting SO_REUSEADDR...OK.\n");

   /* set up destination address */
   memset(&addr_avion, 0, sizeof(addr_avion));
   addr_avion.sin_family=AF_INET;
   addr_avion.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
   addr_avion.sin_port=htons(PORTMULTI);

  /* bind to receive address */
   if (bind(sock, (struct sockaddr *) &addr_avion, sizeof(addr_avion))/* < 0*/) {
      perror("bind");
      close(sock);
      exit(EXIT_FAILURE);
   }
  //else printf("Binding datagram socket...OK.\n");

  /**
   * construct an IGMP join request structure
   * use setsockopt() to request that the kernel join a multicast group
   */
   mreq.imr_multiaddr.s_addr = inet_addr(MULTICASTGROUP);
   mreq.imr_interface.s_addr = htonl(INADDR_ANY);

   /* send an ADD MEMBERSHIP message via setsockopt */
   if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,sizeof(mreq)) < 0) {
      perror("setsockopt");
      close(sock);
      exit(EXIT_FAILURE);
   }
   // else printf("Adding multicast group...OK.\n");

  /***********************************/
  /****** socket multicast crée ******/
  /***********************************/

  /**
   * On va récupérer sockaddr_in crée par le SGCA pour la connexion TCP
   */

   while (a == 0) {
      //memset(addr_SGCA, 0, sizeof(addr_SGCA));
      addrlen=sizeof(addr_envoyeur);
      memset(&addr_envoyeur, 0, addrlen);

      /* block waiting to receive packet */
      nbytes = recvfrom(sock, &addr_SGCA, sizeof(addr_SGCA), 0,
         (struct sockaddr *) &addr_envoyeur, (socklen_t *) &addrlen);
      if (nbytes < 0) {
         perror("recvfrom");
         exit(EXIT_FAILURE);
      }
      printf("Received %d bytes from %s: ", nbytes, inet_ntoa(addr_envoyeur.sin_addr));
      char* ipString = inet_ntoa(addr_envoyeur.sin_addr);
      
      if(ipString) {
         printf("SGCA trouve\n");
         /* send a DROP MEMBERSHIP message via setsockopt */
         if ((setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void*) &mreq, sizeof(mreq))) < 0) {
            perror("setsockopt() failed");
            exit(1);
         }
         fermer_communication();
         a = 1;
         //ouvrir_communication_TCP();
      }
      //printf("x = %d, y = %d, altitude = %d\n",coord.x, coord.y, coord.altitude);
   } /* end while */
   return 1;
}

void fermer_communication()
{
  // fonction à implémenter qui permet de fermer la communication
  // avec le gestionnaire de vols
 close(sock);
}

void envoyer_caracteristiques()
{
   // nombre d'octets envoyés/reçus
   int nb_octets;
   // fonction à implémenter qui envoie l'ensemble des caractéristiques
   // courantes de l'avion au gestionnaire de vols
   if (sendto(sock,&coord,sizeof(struct coordonnees),0,(struct sockaddr *) &addr_SGCA, sizeof(addr_SGCA)) < 0) {
   //send(sock, &coord, sizeof(struct coordonnees), 0);
      perror("send failed");
      exit(EXIT_FAILURE);
   }

}

/********************************
 ***  Fonctions gérant le déplacement de l'avion : ne pas modifier
 ********************************/

// initialise aléatoirement les paramètres initiaux de l'avion
void initialiser_avion()
{
  // initialisation aléatoire du compteur aléatoire
 time_t seed;
 time(&seed);
 srandom(seed);

  // intialisation des paramètres de l'avion
 coord.x = 1000 + random() % 1000;
 coord.y = 1000 + random() % 1000;
 coord.altitude = 900 + random() % 100;

 dep.cap = random() % 360;
 dep.vitesse = 600 + random() % 200;

  // initialisation du numero de l'avion : chaine de 5 caractères
  // formée de 2 lettres puis 3 chiffres
 numero_vol[0] = (random() % 26) + 'A';
 numero_vol[1] = (random() % 26) + 'A';
 sprintf (&numero_vol[2], "%03ld", (random() % 999) + 1);
 numero_vol[5] = 0;
}

// modifie la valeur de l'avion avec la valeur passée en paramètre
void changer_vitesse(int vitesse)
{
 if (vitesse < 0)
  dep.vitesse = 0;
else if (vitesse > VITMAX)
  dep.vitesse = VITMAX;
else dep.vitesse = vitesse;
}

// modifie le cap de l'avion avec la valeur passée en paramètre
void changer_cap(int cap)
{
 if ((cap >= 0) && (cap < 360))
  dep.cap = cap;
}

// modifie l'altitude de l'avion avec la valeur passée en paramètre
void changer_altitude(int alt)
{
 if (alt < 0)
  coord.altitude = 0;
else if (alt > ALTMAX)
  coord.altitude = ALTMAX;
else coord.altitude = alt;
}

// affiche les caractéristiques courantes de l'avion
void afficher_donnees()
{
 printf("Avion %s -> localisation : (%d,%d), altitude : %d, vitesse : %d, cap : %d\n",
   numero_vol, coord.x, coord.y, coord.altitude, dep.vitesse, dep.cap);
}

// recalcule la localisation de l'avion en fonction de sa vitesse et de son cap
void calcul_deplacement()
{
 float cosinus, sinus;
 float dep_x, dep_y;
 int nb;

 if (dep.vitesse < VITMIN)
 {
  printf("Vitesse trop faible : crash de l'avion\n");
  fermer_communication();
  exit(2);
}
if (coord.altitude == 0)
{
  printf("L'avion s'est ecrase au sol\n");
  fermer_communication();
  exit(3);
}

cosinus = cos(dep.cap * 2 * M_PI / 360);
sinus = sin(dep.cap * 2 * M_PI / 360);

dep_x = cos(dep.cap * 2 * M_PI / 360) * dep.vitesse * 10 / VITMIN;
dep_y = sin(dep.cap * 2 * M_PI / 360) * dep.vitesse * 10 / VITMIN;

  // on se déplace d'au moins une case quels que soient le cap et la vitesse
  // sauf si cap est un des angles droit
if ((dep_x > 0) && (dep_x < 1)) dep_x = 1;
if ((dep_x < 0) && (dep_x > -1)) dep_x = -1;

if ((dep_y > 0) && (dep_y < 1)) dep_y = 1;
if ((dep_y < 0) && (dep_y > -1)) dep_y = -1;

  //printf(" x : %f y : %f\n", dep_x, dep_y);

coord.x = coord.x + (int)dep_x;
coord.y = coord.y + (int)dep_y;

afficher_donnees();
}

// fonction principale : gère l'exécution de l'avion au fil du temps
void se_deplacer()
{
 while(1)
 {
  sleep(PAUSE);
  calcul_deplacement();
  envoyer_caracteristiques();
}
}

int main()
{
  // on initialise l'avion
 initialiser_avion();

 afficher_donnees();
  // on quitte si on arrive à pas contacter le gestionnaire de vols
 if (!ouvrir_communication())
 {
  printf("Impossible de contacter le gestionnaire de vols\n");
  exit(1);
}

  // on se déplace une fois toutes les initialisations faites
se_deplacer();
}
