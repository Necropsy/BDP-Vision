#include <stdio.h>

//usando a inplementação de cliente do proprio ssl
#include "robocup_ssl_client.h"

//Bibliotecas geradas pelo padrão de msg protobuf
#include "messages_robocup_ssl_detection.pb.h"
//Caso queira usar informações da geometria, basta
//usar a biblioteca abaixo, para mais inf. vide
//messages_robocup_ssl_geometry.h de como usar
//#include "messages_robocup_ssl_geometry.pb.h"
#include "messages_robocup_ssl_wrapper.pb.h"

//======================Envio de Socket======================
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include <string>

//Parametros para o servidor
#define BDP_Tbuffer 1024
#define PORTA 13822
#define IP "224.5.23.2"
//===========================================================

//Valor ficticio
#define NA 999.0
//Tamanho do vetor BDP_MSG
#define MSG 33

using namespace std;

//Globais:
//BDP_MSG gasta ou todo 33 posições
float BDP_MSG[MSG];
char BDP[100];
int varControle = 0;

//Padrão de msg BDP:
//BOLA:[conf.][posX][posY] TIME1:[id][conf.][posX][posY][ang.] TIME2:[id][conf.][posX][posY][ang]

//Funções:
void RobotInfo(const SSL_DetectionRobot & robot) {

	if (robot.has_robot_id()) {
        BDP_MSG[varControle] = robot.robot_id();
    } else {
        BDP_MSG[varControle] = NA;
    }
    BDP_MSG[varControle+1] = robot.confidence();
    BDP_MSG[varControle+2] = robot.x();
    BDP_MSG[varControle+3] = robot.y();
    if (robot.has_orientation()) {
        BDP_MSG[varControle+4] = robot.orientation();
    } else {
        BDP_MSG[varControle+4] = NA;
    }

    varControle += varControle+5;
}

void toVecChar(){
	char auxMSG[5];
	BDP[0] = '&';
	for(int i=0; i<MSG; i++){
		printf("%d ", int(BDP_MSG[i]));
	}
	int j;
	for(j=0; j<MSG; j++){
		sprintf(auxMSG, "%d", int(BDP_MSG[j]));
		strcat(BDP,auxMSG);
	}
	BDP[j] = '$';
}

int main(){
	//Instanciando um cliente para estabelecer
	//conexão (mais inf. ver robocup_ssl_client.h)
	RoboCupSSLClient client;
	//Neste caso, o cliente ja esta por default
	//conectado no ssl, não é necessario informar
	//o IP e a PORTA (224.5.23.2 , 10006)
    
    client.open(true);
    
    //Instanciando o pacote (obtido dos arq. protobuf)
    SSL_WrapperPacket packet;

    //=======================SERVIDOR=======================
    struct sockaddr_in BDP_Servidor;
    int BDP_Socket;
    char BDP_buffer[BDP_Tbuffer];

    BDP_Socket = socket(PF_INET, SOCK_DGRAM, 0);
    BDP_Servidor.sin_family = AF_INET;
    BDP_Servidor.sin_port = htons(PORTA);
    BDP_Servidor.sin_addr.s_addr = inet_addr("192.168.0.171");

    memset(BDP_buffer, '\0', BDP_Tbuffer);

    //bind(meuSocket, (struct sockaddr*)&servidor, sizeof(sockaddr_in));
    //======================================================

    while(true){
    	
    	//verifica se algum pacote foi recebido
    	if(client.receive(packet)){

    		//Funções obtidas pela biblioteca messages_robocup_ssl_wrapper
    		//verifica se tem informação no pacote
    		if(packet.has_detection()){
    			SSL_DetectionFrame detection = packet.detection();

    			//Estou interessado em capturar apenas 1
    			//para testes com um num. maior basta colocar
    			//em um laço de repetição
    			int balls_n = detection.balls_size();
    			if(balls_n >= 1){
    				SSL_DetectionBall ball = detection.balls(0);
    				BDP_MSG[varControle] = ball.confidence();
    				BDP_MSG[varControle+1] = ball.x();
    				BDP_MSG[varControle+2] = ball.y();
    			}else{
    				//Envia um valor ficticio caso não seja
    				//detectado corretamente
    				BDP_MSG[varControle] = NA;
    				BDP_MSG[varControle+1] = NA;
    				BDP_MSG[varControle+2] = NA;
    			}

    			//Dados da bola completos
    			varControle += varControle+3;

    			//TIME1:
                int robots_blue_n = detection.robots_blue_size();
    			
    			//***Limitando a 3 robos por time (EM ANALISE SE É
    			//***VIÁVEL LIMITAR)
    			//***if(robots_blue_n>3){robots_blue_n = 3;}
    			
    			for(int i=0; i<robots_blue_n; i++){
    				SSL_DetectionRobot robot = detection.robots_blue(i);
    				RobotInfo(robot);
    			}

    			//TIME2:
                int robots_yellow_n = detection.robots_yellow_size();
    			
    			for(int i=0; i<robots_yellow_n; i++){
    				SSL_DetectionRobot robot = detection.robots_yellow(i);
    				RobotInfo(robot);
    			}

    		}else{
    			//Se o pacote não foi montado/recebido corretamente
    			//seta o BDP_MSG com a flag -1 no inicio, esta implementação
    			//foi pensada para calcular futuramente a quantidade de falhas
    			//na obtenção do pacote diretamente do SSL-Vision
    			BDP_MSG[varControle] = -1;
    		}

    		//Print da mensagem criada para debug
    		//printf("======================== Novo Pacote ========================\n");
    		//for(int i=0; i<MSG; i++){
    		//	printf("%f ", BDP_MSG[i]);
    		//}

    		//Envia um pacote no IP e PORTA especificado============================
    		memset(BDP, '\0', 100);
    		toVecChar();
    		int BDP_Envio = sendto(BDP_Socket, BDP, sizeof(BDP), 0, (struct sockaddr*)&BDP_Servidor, sizeof(sockaddr_in));
    		if(BDP_Envio){
    			printf("=======================================Pacote Enviado!=======================================\n");
    		}else{
    			printf("=======================================Falha no Envio!=======================================\n");
    		}
    		memset(BDP_buffer, '\0', BDP_Tbuffer);
    		//======================================================================

    	}
    	//Reseta a variavel de controle para o proximo
    	//pacote que iremos receber
    	varControle = 0;
    }

return 0;
}