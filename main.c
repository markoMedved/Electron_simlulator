//Knjižnjici z openGL funcijami in za ustvarjanje okna
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <math.h>

//Matematične knjižnice za operacije z matrikami in vektorji
#include "cglm/affine.h"
#include "cglm/cam.h"


#define CIRCLE_RENDER_COUNT 1000 //definira koliko trikotnikov bo sestavljalo krog
#define PI 3.14159
#define LENGTH 19200.0f
#define HEIGTH 10800.0f 
#define CM_TO_SCREEN 100.0f //razmerje cm z prikazom na zaslonu
#define e0 1.6f //osnovni naboj
#define m0 9.1f//osnovna masa(ni upoštevano, da je proton veliko težji kot elektron, upošteva se le njun naboj)

 
struct VertexBuffer {
	unsigned int RendererID;//ime tabele, ki vsebuje imena vseh bufferjev z lastnostmi 
	unsigned int size;//velikost podatkov v tabeli

};

//struktura, ki vsebuje indekse ki določajo v kakšnem vrstem redu se bodo izrisovala ogljišča lika
struct IndexBuffer {
	unsigned int RendererID;//Ime tabele v kateri so tabele z indeksi
	unsigned int count;//število elementov ki jih vsebuje IndexBuffer

};


//Funkcija razveže vse bufferje
void unbindAll() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


//Funkcija veže bufferje potrebne za risanje
void bindVertexAndElement(unsigned int vao, unsigned int indexID) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexID);
}

//funkcija naredi strukturo indexBuffer, ki vsebuje indekse ki določajo v kakšnem vrstem redu se bodo izrisovala ogljišča lika
struct IndexBuffer createindexBuffer(unsigned int RendererID[], const unsigned int* data, unsigned int count) {
	struct IndexBuffer buffer;

	glGenBuffers(1, &RendererID);//generira tabelo imen lastnosti(indeksi ogljišč ki se bodo izrisala)
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RendererID);//vežemo buffer

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW);//vpišemo podatke v buffer v tem primeru indekse ogljišč

	//zato da lahko vse vrnemo pod enim imenom vrnemo kot strukturo 
	buffer.RendererID = RendererID;
	buffer.count = count;

	return buffer;
}


//funkcija ustvari Vertex Array, ki vsebuje vse potrebne podatke za izris lika
void createVertexArray(unsigned int* name, GLenum type, unsigned int velikostLastnosti, int componentNumber) {

	glGenVertexArrays(1, name);//pod ime(name) spravi Vertex Array buffer  
	glBindVertexArray(*name);//veže vertex array buffer

	glEnableVertexAttribArray(0);//uporabi Vertex array buffer, ki je trenutno vezan in omogoči lastnost oglišča določeno z indeksom(v mojem primeru indeks 0)

	/*Funkcija sprejme(index tabele lastnosti ki bo uporabljena, stevilo komponent lastnosti ogljišča(npr če je 3D ali 2D),
	tip elementa v tabeli(npr. float),GL_FALSE pove da nočemo normaliziranih vrednosti,
	velikost v bajtih koliko je od enega do naslednjega ogljišča podatkov) in poveže vertex array object z vertex bufferjem */
	glVertexAttribPointer(0, componentNumber, type, GL_FALSE, velikostLastnosti, 0);

}

//funkcija ustvari strukturo VertexBuffer, ki vsebuje lastnosti ogljišča nekega lika(npr. pozicije)
struct VertexBuffer createVertexBuffer(unsigned int RendererID[], const void* bufferData, unsigned int size) {
	struct VertexBuffer buffer;

	glGenBuffers(1, RendererID);//generira tabelo imen lastnosti(RendererID)
	glBindBuffer(GL_ARRAY_BUFFER, RendererID);//vežemo to tabelo
	glBufferData(GL_ARRAY_BUFFER, size, bufferData, GL_STATIC_DRAW);//vpišemo podatke v tabelo(npr. pozicije)

	//zato da lahko vse vrnemo pod enim imenom vrnemo kot strukturo 
	buffer.RendererID = RendererID;
	buffer.size = size;

	return buffer;
}


//Funkcija za izracun pospeska zaradi vpliva elektricnega polja na elektron
float elektricnoPoljePospesek(float razdalja) {
	float F, a;
	float U, d;

	printf("Izbrite in vpisite napetost med elektrodama kondenzatorja[V]: ");
	while (!(scanf("%f", &U))) {
		while (getchar() != '\n') {}
	}

	F = U / (razdalja * 1e-3) * e0;
	a = F / m0;
	a /= 75;

	return a;
}

//Funkcija nariše element podan z pozicijami, indeksi ogljišč in sprejme tudi program(shader), ki izvdede to risanje
void draw(unsigned int* vertexArray, struct IndexBuffer ib, unsigned* shader) {

	//uporabimo program in vežemo vertex array in index buffer
	glUseProgram(*shader);
	bindVertexAndElement(*vertexArray, ib.RendererID);

	//funkcija nariše elemente(osnovno trikotnike) glede na indekse(predstavljajo ogljišča) v index bufferju
	glDrawElements(GL_TRIANGLES, ib.count, GL_UNSIGNED_INT, NULL);

	//vse odvežemo
	unbindAll();
	glDeleteProgram(*shader);
}


//Funkcija ustvari MVP(model-view-projection matrix), ki določajo kje na zaslonu so liki oziroma od kod jih gledamo
// Vsi tipi vec in mat so typedefi iz matematičnih knižnjic
void createMVP(vec3 viewVector, vec3 modelVector, mat4 MVP) {

	mat4 view;//določa od kje gledamo z navidezno kamero
	glm_mat4_identity(view);//naredi matriko->identiteto
	glm_translate(view, viewVector);

	mat4 proj;//Določa razmerje okna in priredi to razmerje likom, dooloča tudi meje kje se lahko riše
	glm_ortho(0.0f, LENGTH , 0.0f, HEIGTH , -1.0f, 1.0f, proj);

	mat4 model;//Določa pozicijo lika, ki se riše 
	glm_mat4_identity(model);
	glm_translate(model, modelVector);

	glm_mat4_mul(proj, view,MVP); //množenje matrik
	glm_mat4_mul(MVP, model, MVP);
}

//Struktura vsebuje vse podatke za risanje
struct data {
	unsigned int vao[5];
	struct IndexBuffer ib;
	struct VertexBuffer vb;
};

//ustvari vse potrebne podatke za izris
void generateData(struct data *data, float positions[], unsigned int indices[], size_t positionsSize, size_t indicesSize) {
	
	unsigned int *bufferIDBuffer = malloc(20);//kazalec na podatke ogljišč(pri meni so to le pozicije ogljišč)
	unsigned int *indexIDBuffer = malloc(20);//kazalec na indekse ogljišč

	data->vb = createVertexBuffer(bufferIDBuffer, positions, positionsSize);
	data->ib = createindexBuffer(indexIDBuffer, indices, indicesSize);

	//(vertex array object)vsebuje enega ali več vertex bufferjev(njihove imena oz naslove)
	createVertexArray(&data->vao, GL_FLOAT, sizeof(float) * 2, 2);

	unbindAll();
	
}


//Funkcija ustavri pozicije ogljišč trikotnikov, ki bodo sestavljali krog
void getCirclePositions(float radius, float positions[]) {
	double angle = 0;
	positions[0] = 0.0f;
	positions[1] = 0.0f;
	//za vsako ogljišče sta dve poziciji(kosinus in sinus kota(od 0 do 2pi v enakomernih skokih), + 4 je zato ker sta vedno 2 ogljišči več kot je trikotnikov)
	for (int i = 2; i < 2 * CIRCLE_RENDER_COUNT + 4; i += 2) {
		positions[i] = radius * cos(angle);
		positions[i + 1] = radius * sin(angle);
		angle += 2 * PI / CIRCLE_RENDER_COUNT;
	}
}

//funkcija ustari indekse ogljišč trikotnikov ki sestavljajo krog
void getCircleIndices(unsigned int indices[]) {
	unsigned int tmp = 1;
	int i = 0;
	//preprosto so po tri stevilke od katerih je ena vedno 0(ogljišče v sredini kroga), drugi dve sta zaporedni
	for (i = 0; i < 3 * CIRCLE_RENDER_COUNT-3; i += 3) {
		indices[i] = 0;
		indices[i + 1] = tmp;
		indices[i + 2] = tmp + 1;
		tmp++;

	}
	//zadnji trikotnik je poseben, ker ga sestavlja tudi ogljišče 1
	indices[i] = 0;
	indices[i + 1] = tmp;
	indices[i + 2] = 1;
}


//Funkcija sprejme tip shaderja in njegovo izvorno kodo in ga prevede(shaderji so napisani v jeziku GLSL zato se posebaj prevajajo)
unsigned int CompileShader(unsigned int type, const char* source) {
	
	unsigned int id = glCreateShader(type);//ustvari prostor za shader in vrne vrednost po kateri se lahko nanj sklicujemo
	const char* src = source;//kazalec na string-source

	glShaderSource(id, 1, &source, NULL);//shaderju, ki smo ga ustvarili, da izvorno kodo iz source
	glCompileShader(id);//prevede shader

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);//vrne GL_TRUE(1) če se je uspešno izvedlo ali GL_FALSE(0), če se ni 

	return id;
}

//Funkcija ustvari Program iz obeh shaderjev
 int CreateShader(const char* vertexShader, const char* fragmentShader) {

	//ustvarjanje programa, ki vsebuje oba shaderja
	unsigned int program = glCreateProgram();

	//prevajanje shaderjov
	unsigned int vershad = CompileShader(GL_VERTEX_SHADER,vertexShader);
	unsigned int fragshad = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	//Pripenjaje obeh shaderjov v program
	glAttachShader(program, vershad);
	glAttachShader(program,fragshad);

	//poveže Shaderja
	glLinkProgram(program);

	return program;
}


 int main(void)
 {
	 printf("Pozdravljeni v simulaciji elektrona/protona v kondenzatorju\n");

 start:
	 printf("\nZa simulacijo elektrona pritisnite 0, za simulacijo protona pritisnite 1\n");
	 _Bool proton = 0;
	 while(!(scanf("%f", &proton))){
		 while (getchar() != '\n') {}
	 }

	 float xHitrost;
	 printf("\nVpisite  vstopno hitrost elektrona/protona[km/s]: ");
	 while (!(scanf("%f", &xHitrost))) {
		 while (getchar() != '\n') {}
	 }
	 

	 float razdalja;
	 printf("Vpisite razdaljo med elektrodama kondenzatorja[cm](max %.0f): ", HEIGTH / CM_TO_SCREEN);//če bo razdalja večja, se kondenzator ne bo izrisal
	 while (!(scanf("%f", &razdalja))) {
		 while (getchar() != '\n') {}
	 }

	 float delta = 0;//spremenljivka, ki bo opisovala navpično pot
	 float time = 1.0f/60;//toliko časa poteče med vsako ponovno spremembo zaslona
	 float accel = elektricnoPoljePospesek(razdalja);

	 GLFWwindow* window;

	 //Inicializiranje knjižnice glfw
	 if (!glfwInit())
		 return -1;

	 // izdelava okna za prikaz simulacije
	 window = glfwCreateWindow(LENGTH / 10, HEIGTH / 10, "Simulacija(Press space to start)", NULL, NULL);//sprejme(velikost, ime, če hočemo fullscreen)

	 if (!window) //če ni uspelo okno, se konča program
	 {
		 glfwTerminate();
		 return -1;
	 }

	 glfwMakeContextCurrent(window);

	 glfwSwapInterval(1);//sinhronizira upodabljanje računalnika z monitorjem

	 if (glewInit() != GLEW_OK) //inicializacija knjižnice glew
		 printf("Error");


	 //Shader-program, ki poskrbi za izrisovanje likov na zaslon(napisan v jeziku GLSL) in se izvaja na grafični kartici

	 //Shader, ki se izvede za vsako ogljišča(Določa kje je tvoj lik na zaslonu)
	 char* vertexShader = "#version 330 core\n"
		 "\n"
		 "layout(location = 0)in vec4 position;\n"//shader sprejme input pozicij(layout določa indeks lastnosti(pri meni so le pozicije zato samo 0)) 
		 "uniform mat4 u_MVP;\n"//model view projection matrix-določa pozicijo modela(ki ga rišemo), 
		 "void main()\n"
		 "{\n"
		 "	gl_Position = u_MVP * position;\n"//Preko uniforme v shaderju menjamo lokacije modelov
		 "};\n";

	 //Shader, ki se izvede za vsak pixel(Določa barvo pixla)
	 char* fragmentShader = "#version 330 core\n"
		 "layout(location = 0) out vec4 color;\n"//Shader odda barvo ki jo določimo preko uniforme
		 "uniform vec4 u_Color;\n"
		 "void main()\n"
		 "{\n"
		 "color = u_Color;\n"//Preko uniforme menjamo barvo vsakega lika
		 "};\n";

	 unsigned int shader = CreateShader(vertexShader, fragmentShader);

	 //Uniforme so način, da v shaderju zamenjamo določene podatke med izvajanjem programa
	 //Uniforma za barvo
	 int location = glGetUniformLocation(shader, "u_Color");
	
	 //Uniforma za model-view-projection matrix
	 int loc = glGetUniformLocation(shader, "u_MVP");
	

	 //Pove, kam se prestavi navidezna kamera
	 vec3 viewVec = { 0, 0, 0 };

	 //začetna pozicija delca
	 float xModel = 0, yModel = HEIGTH / 2;

	 mat4 MVP;

	 //podatki za delec(krog na zaslonu)
	 float radius = 200.0f;
	 float positions[2 * CIRCLE_RENDER_COUNT + 4];
	 getCirclePositions(radius, positions);
	 unsigned int indices[3 * CIRCLE_RENDER_COUNT];
	 getCircleIndices(indices);

	 struct data data1;
	 generateData(&data1, positions, indices, sizeof(positions), sizeof(indices) / sizeof(unsigned int));


	 //podatki za minus v krogu
	 float positions2[] = {
		 -100.0f, 30.0f,
		 100.0f, 30.0f,
		 -100.0f, -30.0f,
		 100.0f, -30.0f
	 };

	 unsigned int indices2[] = {
		0,1,2,
		1,2,3

	 };

	 //krog se obarva modro, če je elektron ali rdeče, če je proton
	 float red, blue;
	 if (proton) {
		 red = 1.0f;
		 blue = 0.0f;
	 }
	 else {
		 red = 0.0f;
		 blue = 1.0f;
	 }

	 struct data data2;
	 generateData(&data2, positions2, indices2, sizeof(positions2), sizeof(indices2) / sizeof(unsigned int));

	 //dodatek za plus
	 float positions2b[] = {
		 -30.0f, 100.0f,
		 30.0f, 100.0f,
		 -30.0f, -100.0f,
		 30.0f, -100.0f
	 };

	 struct data data2b;
	 generateData(&data2b, positions2b, indices2, sizeof(positions2b), sizeof(indices2) / sizeof(unsigned int));


	 //podatki za kondenzator-prijemali
	 float positions3[] = {
		450.0f, (HEIGTH / 4 - CM_TO_SCREEN * razdalja / 4),
		450.0f, -(HEIGTH / 4 - CM_TO_SCREEN * razdalja / 4),
		-450.0f,(HEIGTH / 4 - CM_TO_SCREEN * razdalja / 4),
		-450.0f, -(HEIGTH / 4 - CM_TO_SCREEN * razdalja / 4)

	 };

	 unsigned int indices3[] = {
		 0,1,2,
		 1,2,3
	 };

	 struct data data3;
	 generateData(&data3, positions3, indices3, sizeof(positions3), sizeof(indices3) / sizeof(unsigned int));


	 //podatki za kondenzator-plošči
	 float positions4[] = {
		-LENGTH / 2, 50.0f,
		-LENGTH / 2, -50.0f,
		LENGTH / 2, 50.0f,
		LENGTH / 2, -50.0f
	 };

	 unsigned int indices4[] = {
		0,1,2,
		1,2,3
	 };

	 //glede na predznak napetosti(posledično pospeška) se plošči obarvata(modro negativno, rdeče pozitivno)
	 float col1, col2;
	 if (accel > 0) {
		 col1 = 1.0f;
		 col2 = 0.0f;
	 }
	 else {
		 col1 = 0.0f;
		 col2 = 1.0f;
	 }

	 struct data data4;
	 generateData(&data4, positions4, indices4, sizeof(positions4), sizeof(indices4) / sizeof(unsigned int));


	 //podatki za črni zaslon na koncu kondenzatorja
	 float positions5[] = {
		 50.0f, (CM_TO_SCREEN * razdalja / 2),
		 50.0f,-(CM_TO_SCREEN * razdalja / 2),
		 - 50.0f,(CM_TO_SCREEN * razdalja / 2),
		 - 50.0f,-(CM_TO_SCREEN * razdalja / 2)
	 };

	 struct data data5;
	 generateData(&data5, positions5, indices4, sizeof(positions5), sizeof(indices4) / sizeof(unsigned int)); 


	 //če je proton obrne smer pospeška
	 if (proton) accel *= -1;

	 int space = 0;

	 // Zanka dokler se okno ne zapre
	 while (!glfwWindowShouldClose(window))
	 {
		 
		 glClear(GL_COLOR_BUFFER_BIT);//pobriše prejšnji zaslon(barve)
		 glClearColor(0.53f, 0.81f, 0.92f, 1.0f);//nastavi barvo zaslona

		 //za risanje vsake vrste lika je svoj scope
		 {
			//risanje kroga za delec
			 vec3 modelVec = {xModel, yModel, 0 };

			 createMVP(viewVec, modelVec, MVP);

			 glUniform4f(location, red, 0.0f, blue, 1.0f);
			 glUniformMatrix4fv(loc, 1, GL_FALSE, MVP);

			 draw(&data1.vao, data1.ib, &shader);
		 }

		 {
			 //risanje minusa ali plusa(za proton) za delec
			 vec3 modelVec = { xModel, yModel, 0 };

			 createMVP(viewVec, modelVec, MVP);

			 glUniform4f(location, 0, 0, 0.0f, 1.0f);
			 glUniformMatrix4fv(loc, 1, GL_FALSE, MVP);

			 draw(&data2.vao, data2.ib, &shader);

			 if (proton) {
				 draw(&data2b.vao, data2b.ib, &shader);
			 }

		 }

		 {
			 //risanje prijemal kondenzatorja
			 vec3 modelVec = { LENGTH / 2, 3 * HEIGTH / 4 + razdalja * CM_TO_SCREEN / 4, 0 };
			 createMVP(viewVec, modelVec, MVP);

			 glUniform4f(location, 0.5f, 0.5f, 0.5f, 1.0f);
			 glUniformMatrix4fv(loc, 1, GL_FALSE, MVP);

			 draw(&data3.vao, data3.ib, &shader);


			 vec3 modelVec2 = { LENGTH / 2,HEIGTH / 4 - CM_TO_SCREEN * razdalja / 4 , 0 };
			 createMVP(viewVec, modelVec2, MVP);

			 glUniformMatrix4fv(loc, 1, GL_FALSE, MVP);

			 draw(&data3.vao, data3.ib, &shader);

		 }

		 {
			 //risanje plošč kondenzatorja
			 vec3 modelVec = { LENGTH / 2, HEIGTH / 2 + razdalja * CM_TO_SCREEN / 2 };

			 createMVP(viewVec, modelVec, MVP);

			 glUniform4f(location, col1, 0.0f, col2, 1.0f);
			 glUniformMatrix4fv(loc, 1, GL_FALSE, MVP);

			 draw(&data4.vao, data4.ib, &shader);

			 vec3 modelVec2 = { LENGTH / 2,HEIGTH / 2 - razdalja * CM_TO_SCREEN / 2, 0 };
			 createMVP(viewVec, modelVec2, MVP);

			 glUniform4f(location, col2, 0.0f, col1, 1.0f);
			 glUniformMatrix4fv(loc, 1, GL_FALSE, MVP);

			 draw(&data4.vao, data4.ib, &shader);

		 }
		 {
			 //risanje črnega zaslona
			 vec3 modelVec = {LENGTH - 50.0f, HEIGTH/2, 0};
			 createMVP(viewVec, modelVec, MVP);

			 glUniform4f(location, 0, 0, 0, 1.0f);
			 glUniformMatrix4fv(loc, 1, GL_FALSE, MVP);

			 draw(&data5.vao, data5.ib, &shader);
		 
		 
		 }

		 //simulacija se bo začela s pritiskom na space
		 if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			 space = 1;
		 }

		 if (space) {
			 //If stavek določa kdaj se bo delec ustavil(če pride do zaslona ali kondenzatorja)
			 if (xModel + radius < LENGTH && yModel + radius < HEIGTH / 2 + razdalja * CM_TO_SCREEN / 2 && yModel - radius > HEIGTH / 2 - razdalja * CM_TO_SCREEN / 2) {
				 xModel += xHitrost;//hitrost v smeri x je konstantna
				 yModel += delta;
				 delta += accel * time;//hitrost v smeri y se linearno povečuje(zato delec potuje po paraboli)
			 }
		 }
		 // menjava sprednjega in zadnjega bufferja okna
		 glfwSwapBuffers(window);

		 //Posodobi stanje okna
		 glfwPollEvents();
		 
	
	 }

	 glfwTerminate();

	 if (xModel + radius + 1.0f >= LENGTH) {
		 if (proton) printf("\nproton je pristal %.2f cm od sredisca crnega zaslona\n", (yModel - HEIGTH / 2) / CM_TO_SCREEN);
		 else printf("\nelektron je pristal %.2f cm od sredisca zaslona\n", (yModel- HEIGTH / 2) / CM_TO_SCREEN);
	 }
	 else {
		 if (proton) printf("\nproton je pristal na kondenzatorju %.2f cm od zacetka kondenzatorja\n", xModel / CM_TO_SCREEN);
		 else printf("\nelektron je pristal na kondenzatorju %.2f cm od zacetka kondenzatorja\n", xModel / CM_TO_SCREEN);
	 }

	 int tmp;
	 printf("\nIzberi stevilko moznosti:\n1)Ponovna simulacija\n2)Izhod\n");
	 while (!(scanf("%d", &tmp))) {
		 while (getchar() != '\n') {}
	 }
	 if (tmp == 1) {
		 goto start;
	}

	printf("\nHvala, da ste uporabili mojo simulacijo\nMarko Medved\n");
	
	system("pause");

	return 0;
}