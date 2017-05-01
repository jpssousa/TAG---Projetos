/*
2017-1
Teoria e Aplica��o de Grafos - Turma A
Prof. D�bio Borges

Aluno: Cristiano Cardoso 		15/0058349
Aluno: Jo�o Pedro Silva Sousa	15/0038381

0. Requisitos
cristian@Jasmine-VirtualBox:/media/sf_TAG-projetos$ gcc --version
gcc (Ubuntu 4.9.4-2ubuntu1~14.04.1) 4.9.4

1. Compila��o
	- V� ate o diretorio de onde foi extraido o programa e entre com o comando
		$ make makefile
	- Um execut�vel entitulado mwfacil deve ser gerado nesta pasta
2. Execu��o
	- Entre no terminal
		$ ./mwfacil
	- Caos queira salvar a saida em um arquivo fa�a
		$ ./mwfacil > saida.txt
3. Sa�da
	- A sa�da consiste em duas partes: uma contendo a ordena��o topol�gica (OT), e a outra, o caminho cr�tico (CP)
		* A OT � elaborada considerando o semestre da disciplina. Ou seja, a disciplina do primeiro semestre dever� ser feita antes de uma situada no terceiro semeste - logo, ficar� na frente.
		� impressa uma lista de prioridade contendo as disciplinas. Isto �, a disciplina devem ser feitas na ordem que aparecem na tela. Exemplo: 1. APC -> 2. ISC -> 3. C1 -> ...
		* O CP � elaborado a partir da ordena��o topol�gica. Ela considera os pesos das disciplinas e mostra qual � a cadeia mais trabalhosa do curso - isto �: onde h� a maior s�rie de mat�rias que dependem uma das outras e sejam consideradas dif�ceis (possuem pesos maiores). Caso o aluno n�o as priorize, haver� maior chance de o tempo de curso atrasar.
*/
#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <string>
#include <regex>

using namespace std;

#define IN_FILE			"disciplinas-tag-2017-1.csv"
#define IN_FILE_FLUXO	"fluxo-tag-2017-1.csv"
#define APP_VERSION		0.1
#define APP_NAME		"MatriculaWeb Facil"
#define FATAL_ERR		-1

/*
Caso esta vari�vel seja verdadeira, todos os detalhes referentes aos algoritmos executados ser�o mostrados
� recomendado redirecionar a saida (stdout) para um arquivo quando esta estiver habilitada
*/
bool verboseEnabled = false;
void printVerbose(string str) {
	if (verboseEnabled)
		cout << str << endl;
}

/*
Traduz a equival�ncia entre F(�cil), M(�dio) e D(if�cil) para um fator de multiplica��o f. Isto serve para evitar com que seja necess�rio ficar digitando os valores decimais na planilha e ter chance de errar.
*/
map<string, float> factorMap = {
	make_pair("F", 0.5),
	make_pair("M", 1.0),
	make_pair("D", 1.5)
};

/*
Elemento n� do grafo: possui os atributos b�sicos de uma disciplina
*/
class Disciplina {
	public:
		string	cod;
		string	nome;
		float	peso;

		Disciplina(string cod, string nome, float peso) {
			this->cod = cod;
			this->nome = nome;
			this->peso = peso;
		}

		string tostring();
};

string Disciplina::tostring() {
	return cod + " - " + nome;
}


/*
Grafo contendo as disciplinas. O seu nome se deve porque estas disciplinas s�o retiradas do fluxo da disciplina.
Seu m�todo para armazenar as arestas � a matriz de adjac�ncia.
Todo v�rtice (Disciplina) recebe um inteiro positivo que o identifica. Assim evita-se ter problemas com ponteiros.
Devido a isto, o n�mero de v�rtices deve ser informado na hora de instanciar o grafo.

Caso uma disciplina A tenha pr�-requisito B, uma aresta de A para B � criada ou matrizAdjancencia[A][B] = 1
*/
class fluxoDisciplina {
	private:
		int			numElementos;
		int			**matrizAdjacencia;
		Disciplina	**vertices;
		int		*elementPool; /*pool of cute elements: they can be destroyed all at once and malloc won't slow down our furious application*/

	public:
		fluxoDisciplina(int numElementos) {
			if (numElementos < 0)
				throw invalid_argument("numElementos must be positive");

			matrizAdjacencia = (int**)malloc(numElementos * sizeof(int*));
			elementPool = (int*)calloc(pow(numElementos, 2), sizeof(int));
			if (matrizAdjacencia == NULL || elementPool == NULL) {
				free(matrizAdjacencia);
				throw bad_alloc();
			}
			for (int i = 0; i < numElementos; i++) {
				matrizAdjacencia[i] = &elementPool[i * numElementos];
			}

			vertices = (Disciplina**)calloc(numElementos, sizeof(Disciplina*));

			this->numElementos = numElementos;
		}
		~fluxoDisciplina() {
			free(matrizAdjacencia); free(elementPool);

			for (int idx = 0; idx < numElementos; idx++) {
				delete vertices[idx];
			}
		}

		bool		addEdge(int a, int b);
		bool		removeEdge(int a, int b);
		Disciplina	*getElement(int id);
		void		setElement(int id, Disciplina *d);
		int			getElementCount();
		void		clone(fluxoDisciplina &dest);
		bool		hasPreRequisitos(int id);
		void		getEdgesFrom(int elemID, list<int> &result);
		void		getDependencias(int elemID, list<int> &result);
};

bool fluxoDisciplina::addEdge(int a, int b) {
	if ( !(0 <= a && a < numElementos) || !(0 <= b && b < numElementos) ){
		return false;
	}

	matrizAdjacencia[a][b] = 1;

	return true;
}

bool fluxoDisciplina::removeEdge(int a, int b) {
	if (!(0 <= a && a < numElementos) || !(0 <= b && b < numElementos)) {
		return false;
	}

	matrizAdjacencia[a][b] = 0;

	return true;
}

/*
Retorna um ponteiro para disciplina dado um ID
O elemento � retirado do reposit�rio (pool) de v�rtices
*/
Disciplina *fluxoDisciplina::getElement(int id) {
	if (! (0 <= id && id < numElementos)) {
		throw invalid_argument(string(__func__) + " id(" + to_string(id) + ") is out of range [0," + to_string(numElementos) + ")");
	}

	return vertices[id];
}

/*
Substitui o objeto armazenado dado um ID
*/
void fluxoDisciplina::setElement(int id, Disciplina *d) {
	if (!(0 <= id && id < numElementos)) {
		throw invalid_argument(string(__func__) + " id(" + to_string(id) + ") is out of range [0," + to_string(numElementos) + ")");
	}

	vertices[id] = d;
}

/*
Retorna o n�mero de v�rtices do grafo
*/
int fluxoDisciplina::getElementCount() {
	return numElementos;
}

/*
Copia as informa��es de um grafo existente para um grafo rec�m criado.
Isto inclui a matriz de adjac�ncia e as informa��es com respeito as disciplinas
O tamanho do grafo de destino deve ser >= ao de origem
O grafo de origem pode ser excluido normalmente, pois de destino realizou uma c�pia de todas suas informa��es.
*/
void fluxoDisciplina::clone(fluxoDisciplina &dest) {
	if (this->numElementos > dest.numElementos) {
		throw invalid_argument("destination element does not have enough space to clone source data.");
	}

	/*adjacency matrix copy routine*/
	for (int i = 0; i < this->numElementos; i++) {
		for (int j = 0; j < this->numElementos; j++) {
			dest.matrizAdjacencia[i][j] = this->matrizAdjacencia[i][j];
		}
	}

	/*vertex copy routine*/
	Disciplina *cur;
	for (int i = 0; i < this->numElementos; i++) {
		cur = this->vertices[i];
		dest.vertices[i] = new Disciplina(cur->cod, cur->nome, cur->peso);
	}
}

/*
Verifica se uma dada disciplina possui pr�-requisitos
*/
bool fluxoDisciplina::hasPreRequisitos(int id) {
	int *requisitosArray;

	requisitosArray = matrizAdjacencia[id];
	for (int idx = 0; idx < numElementos; idx++) {
		if (requisitosArray[idx] == 1) {
			return true;
		}
	}
	return false;
}

/*
Verifica quais s�o os pr�-requisitos de uma dada disciplina
*/
void fluxoDisciplina::getEdgesFrom(int elemID, list<int> &result) {
	if (!(0 <= elemID && elemID < numElementos)) {
		throw invalid_argument(string(__func__) + " id(" + to_string(elemID) + ") is out of range [0," + to_string(numElementos) + ")");
	}

	for (int destID = 0; destID < numElementos; destID++) {
		if (matrizAdjacencia[elemID][destID] == 1) {
			result.push_back(destID);
		}
	}
}

/*
Verifica quais s�o as disciplinas que a dada disciplina � pr�-requisito
*/
void fluxoDisciplina::getDependencias(int elemID, list<int> &result) {
	if (!(0 <= elemID && elemID < numElementos)) {
		throw invalid_argument(string(__func__) + " id(" + to_string(elemID) + ") is out of range [0," + to_string(numElementos) + ")");
	}

	for (int cur = 0; cur < numElementos; cur++) {
		if (matrizAdjacencia[cur][elemID] == 1) {
			result.push_back(cur);
		}
	}

}


class PathNode {
	public:
		int weight;
		int dID;

		PathNode(int weight, int dID) {
			this->weight = weight;
			this->dID = dID;
		}
};

/*
Classe que cont�m toda a l�gica de gerar os caminhos cr�ticos e a ordena��o topol�gica
Seu nome se deve porque esta funciona como se fosse um agendador de tarefas, onde nele � possivel ver quais tarefas (disciplinas) devem ser feito em ordem, e quais delas s�o mais trabalhosas
O scheduler est� intriscamente ligado ao curso, porque com esta informa��o � possivel saber quais disciplinas pertencem
a qual semestre (tamb�m por quest�es de desenvolvimento).
*/
class disciplinaScheduler {
	int courseID;
	map<string, unsigned> semestreOF; /*from an courseID i can know what semestrer it is from*/

	public:
		disciplinaScheduler(int courseID) {
			this->courseID = courseID;
		}
	int		readSemestreDataFromFile(string fileName);
	bool	toLinearSequence(fluxoDisciplina &container, list<int> &dest);
	void	getCriticalPath(fluxoDisciplina &source, list<int> &path, list<PathNode*> &dest);
	private:
		void	printList(fluxoDisciplina &container, list<int> l);
		void	getSortedVerticesList(fluxoDisciplina &source, list<int> &dest);
		void	insertSortedVertex(fluxoDisciplina &source, list<int> &dest, int elemID);
};

/*
Recebe um arquivo contendo as disciplinas discriminadas por semestre a fim de preencher o map semestreOF
*/
int disciplinaScheduler::readSemestreDataFromFile(string fileName) {
	string		line;
	ifstream	inFile(fileName);
	regex		headerHandler("(\\d+),[ ]*?(\\d+)");
	regex		lineHandler("(\\d+),?");
	smatch		match_results;

	printVerbose("(I)::" + string(__func__) + " Attempting to read input file " + fileName);
	if (!inFile) {
		throw ifstream::failure("Fail to open " + fileName);
	}

	getline(inFile, line); /*read header*/
	int courseID, periodNumber;
	if (!regex_match(line, match_results, headerHandler) || match_results.size() < 3) {
		throw domain_error("Bad header in input file " + fileName);
	}
	courseID = stoi(match_results.str(1));
	periodNumber = stoi(match_results.str(2));
	if (periodNumber < 0)
		throw invalid_argument("periodNumber must be positive");
	if (courseID != this->courseID) {
		throw invalid_argument("file courseID ("+to_string(courseID)+") does not match disciplinaScheduler object one ("+to_string(this->courseID)+").");
	}

	int idx = 0; string disciplinaID;
	while (getline(inFile, line)) {
		idx++;

		while (regex_search(line, match_results, lineHandler)) {
			disciplinaID = match_results.str(1);
			semestreOF[disciplinaID] = idx;

			line = match_results.suffix().str();
		}
	}

	printVerbose("(I)::" + string(__func__) + " Read " + to_string(semestreOF.size()) + " disciplina elements");

	return 0;
}

/*
Fun��o de Debug: dada uma lista de disciplinaID, esta � impressa na tela onde cada item � seu c�digo
*/
void disciplinaScheduler::printList(fluxoDisciplina &container, list<int> l) {
	list<int>::iterator it;
	string line;

	for (it = l.begin(); it != l.end(); it++) {
		line += container.getElement(*it)->cod + ", ";
	}
	cout << line << endl;
}

/*
Realiza a ordena��o topol�gica
Fonte: https://en.wikipedia.org/wiki/Topological_sorting#Kahn.27s_algorithm
Obs: O conjunto S � implementado como sendo uma lista de prioridade, onde a disciplina que � de um semestre anterior possui prioridade
*/
bool disciplinaScheduler::toLinearSequence(fluxoDisciplina &container, list<int> &dest) {
	list<int> s;
	fluxoDisciplina instance(container.getElementCount());

	container.clone(instance);
	this->getSortedVerticesList(instance, s);

	printVerbose("(I)::" + string(__func__) + " Initializating Topologic Sorting");
	printVerbose("(I)::" + string(__func__) + " Input S:");
	if (verboseEnabled) {
		printList(instance, s);
	}

	list<int>::iterator it;
	int cur; list<int> adjList;
	while (! s.empty()) {
		cur = s.front(); s.pop_front();
		dest.push_back(cur);

		instance.getDependencias(cur, adjList);
		for (list<int>::iterator it = adjList.begin(); it != adjList.end(); it++) {
			instance.removeEdge(*it, cur);
			if (! instance.hasPreRequisitos(*it)) {
				insertSortedVertex(instance, s, *it);
			}
		}

		adjList.clear();
	}

	printVerbose("(I)::" + string(__func__) + " Output L:");
	if (verboseEnabled) {
		printList(instance, dest);
	}

	return true;
}

/*
Algoritmo de Kahn
Raliza a inser��o de uma disciplinaID em um conjunto S levando em conta a prioridade
*/
void disciplinaScheduler::insertSortedVertex(fluxoDisciplina &source, list<int> &dest, int elemID) {
	Disciplina *cur;
	list<int>::iterator it;

	cur = source.getElement(elemID);

	for (it = dest.begin(); it != dest.end(); it++) {
		if (semestreOF[cur->cod] < semestreOF[source.getElement(*it)->cod]) {
			dest.insert(it, elemID); break;
		}
	}
	if (it == dest.end()) {
		dest.push_back(elemID);
	}
}

/*
Algoritmo de Kahn
Obtem um conjunto S de todas as disciplinas que n�o possuem pr�-requisitos, levando em considera��o a prioridade
*/
void disciplinaScheduler::getSortedVerticesList(fluxoDisciplina &source, list<int> &dest) {
	int max = source.getElementCount();
	Disciplina *cur;
	list<int>::iterator it;


	for (int i = 0; i < max; i++) {
		if (source.hasPreRequisitos(i)) {
			continue;
		}
		cur = source.getElement(i);

		for (it = dest.begin(); it != dest.end(); it++) {
			if (semestreOF[cur->cod] < semestreOF[source.getElement(*it)->cod]) {
				dest.insert(it, i); break;
			}
		}
		if (it == dest.end()) {
			dest.push_back(i);
		}
	}
}

/*
Obt�m o caminho cr�tico
Fonte: https://en.m.wikipedia.org/wiki/Longest_path_problem#Acyclic_graphs_and_critical_paths
*/
void disciplinaScheduler::getCriticalPath(fluxoDisciplina &source, list<int> &path, list<PathNode*> &dest) {
	Disciplina *cur;
	list<int>::iterator it, cit;

	map<int, int> cp_weight;
	map<int, int> cp_previous;
	int cp_max_weight_id = -1; cp_weight[cp_max_weight_id] = 0;

	for (it = path.begin(); it != path.end(); it++) {
		if (verboseEnabled) {
			cout << "it: " << source.getElement(*it)->cod << " - " + source.getElement(*it)->nome << endl;
		}
		cur = source.getElement(*it);
		list<int> dependencias;
		source.getEdgesFrom(*it, dependencias);

		int elem_critico = -1;
		for (cit = dependencias.begin(); cit != dependencias.end(); cit++) {
			if (verboseEnabled) {
				cout << "\tdID: " << source.getElement(*cit)->cod << " - " + source.getElement(*cit)->nome << endl;
			}

			if (cp_weight[*cit] > cp_weight[elem_critico]) {
				elem_critico = *cit;
			}
		}

		cp_weight[*it] = cur->peso + cp_weight[elem_critico];
		cp_previous[*it] = elem_critico;

		if (verboseEnabled) {
			string cpp = cp_previous[*it] != -1 ? source.getElement(cp_previous[*it])->cod : "-1";
			string ec = elem_critico != -1 ? source.getElement(elem_critico)->cod : "-1";
			cout << "\teC: " << ec << "cpW: " << cp_weight[*it] << " cpP: " << cpp << endl;
		}

		if (cp_weight[*it] > cp_weight[cp_max_weight_id]) {
			cp_max_weight_id = *it;
		}
	}

	int cp_cur = cp_max_weight_id;
	while (cp_cur != -1) {
		dest.push_front(new PathNode(cp_weight[cp_cur], cp_cur));

		cp_cur = cp_previous[cp_cur];
	}
}

/*
L� o arquivo contendo disciplinas e seus pesos
*/
bool parseDisciplinasFile(ifstream *infile, fluxoDisciplina &container) {
	string		line;
	const char	infoPattern[] = "(\\d{6}),([\\w ]+),(\\d+),([FMD ])(,[\\d, ]*)";
	const char	numberPattern[] = "\\d+";
	regex		infoHandler(infoPattern); /*function called just one time: no problems to create the object here*/
	regex		requisitosHandler(numberPattern);


	match_results<string::const_iterator> match_itr; int idx = 0; int lineNumber = 1;
	smatch::iterator it;

	string cod, nome, requisitosLine; int creditos; float peso;
	printVerbose("(I) Reading elements from file (Codigo (ID) - Peso (P) - Requisitos (R))");
	while (getline(*infile, line)) {
		regex_match(line, match_itr, infoHandler);

		if (match_itr.empty() || match_itr.size() < 6 || match_itr.str(4) == " ") {
			if (verboseEnabled) {
				cout << "(I) Ignorando linha " << lineNumber << endl; lineNumber++;
			}
			continue;
		}

		cod = match_itr.str(1);
		nome = match_itr.str(2);
		creditos = stoi(match_itr.str(3)); /*can raise an invalid_argument execption*/
		requisitosLine = match_itr.str(5);

		peso = (float)creditos * factorMap[match_itr.str(4)];

		container.setElement(idx, new Disciplina(cod, nome, creditos));

		string reqList = "";
		while ( regex_search(requisitosLine, match_itr, requisitosHandler) ) {
			reqList += match_itr.str(0) + "; ";
			if (! container.addEdge(idx, stoi(match_itr.str(0)))) {
				printVerbose("(W) Could not add edge between " + to_string(idx) + " and " + match_itr.str(0) + ".");
			}
			requisitosLine = match_itr.suffix().str();
		}

		printVerbose(cod + " - " + to_string(peso) + " - " + reqList);

		idx++; lineNumber++;
	}

	return true;
}

/*
Imprime um relat�rio
*/
void printRelatorio(fluxoDisciplina &fd, list<int> tp, list<PathNode*> cp) {
	list<PathNode*>::iterator it;
	list<int>::iterator dit;
	int pos;

	cout << "~! Ordenacao Topologica" << endl; pos = 1;
	for (dit = tp.begin(); dit != tp.end(); dit++) {
		cout << pos << ".\t" << fd.getElement(*dit)->tostring() << endl;

		pos++;
	}
	cout << endl << endl;

	cout << "~! Caminho Critico ~" << endl; pos = 1; PathNode *cur;
	for (it = cp.begin(); it != cp.end(); it++) {
		cur = *it; list<int> dependencias;

		cout << pos << ". " << fd.getElement(cur->dID)->tostring() << endl;

		fd.getEdgesFrom(cur->dID, dependencias);
		for (dit = dependencias.begin(); dit != dependencias.end(); dit++) {
			cout << "\tPre-Req: " << fd.getElement(*dit)->tostring() << endl;
		}

		pos++;
	}
}

/*
Fun��o principal
*/
int main()
{
	cout << APP_NAME << " v" << APP_VERSION << endl;

	ifstream fileHandler(IN_FILE);

	cout << "(I) A carregar dados de " << IN_FILE << ", " << IN_FILE_FLUXO << " ..." << endl;
	cout << "\t Curso: 1856 - Ciencia da Computacao\n" << endl;

	if (!fileHandler) {
		cout << "Nao foi possivel abrir arquivo de entrada " << IN_FILE << ". Abortando operacao." << endl;
		return FATAL_ERR;
	}

	fluxoDisciplina fp(35); /*we already know how many elements we're going work with, but it would be a better if this is told on the header*/
	bool parseSucess = parseDisciplinasFile(&fileHandler, fp);


	list<int> topological_path; list<PathNode*> critical_path;
	disciplinaScheduler dps(1856);
	dps.readSemestreDataFromFile(IN_FILE_FLUXO);
	dps.toLinearSequence(fp, topological_path);
	dps.getCriticalPath(fp, topological_path, critical_path);

	printRelatorio(fp, topological_path, critical_path);

    return 0;
}