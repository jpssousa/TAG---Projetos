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


bool verboseEnabled = false;
void printVerbose(string str) {
	if (verboseEnabled)
		cout << str << endl;
}


map<string, float> factorMap = {
	make_pair("F", 0.5),
	make_pair("M", 1.0),
	make_pair("D", 1.5)
};

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

class fluxoDisciplina {
	int			numElementos;
	int			**matrizAdjacencia;
	Disciplina	**vertices;

	private:
		int		*elementPool;

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

Disciplina *fluxoDisciplina::getElement(int id) {
	if (! (0 <= id && id < numElementos)) {
		throw invalid_argument(string(__func__) + " id(" + to_string(id) + ") is out of range [0," + to_string(numElementos) + ")");
	}

	return vertices[id];
}

void fluxoDisciplina::setElement(int id, Disciplina *d) {
	if (!(0 <= id && id < numElementos)) {
		throw invalid_argument(string(__func__) + " id(" + to_string(id) + ") is out of range [0," + to_string(numElementos) + ")");
	}

	vertices[id] = d;
}

int fluxoDisciplina::getElementCount() {
	return numElementos;
}

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
	//TODO: this part we should clone also each Disciplina element to avoid the dest have dead pointers if 'this' is deleted
	Disciplina *cur;
	for (int i = 0; i < this->numElementos; i++) {
		cur = this->vertices[i];
		dest.vertices[i] = new Disciplina(cur->cod, cur->nome, cur->peso);
	}
}

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

class disciplinaScheduler {
	int courseID;
	map<string, unsigned> semestreOF;

	public:
		disciplinaScheduler(int courseID) {
			this->courseID = courseID;
		}
	int		readSemestreDataFromFile(string fileName);
	bool	toLinearSequence(fluxoDisciplina &container, list<int> &dest);
	void	printList(fluxoDisciplina &container, list<int> l);
	void	getCriticalPath(fluxoDisciplina &source, list<int> &path, list<PathNode*> &dest);
	private:
		void	getSortedVerticesList(fluxoDisciplina &source, list<int> &dest);
		void	insertSortedVertex(fluxoDisciplina &source, list<int> &dest, int elemID);
};

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

void disciplinaScheduler::printList(fluxoDisciplina &container, list<int> l) {
	list<int>::iterator it;
	string line;

	for (it = l.begin(); it != l.end(); it++) {
		line += container.getElement(*it)->cod + ", ";
	}
	cout << line << endl;
}

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

void printRelatorio(fluxoDisciplina &fd, list<int> tp, list<PathNode*> cp) {
	list<PathNode*>::iterator it;
	list<int>::iterator dit;
	int pos;

	cout << "~! Ordenacao Topologica" << endl; pos = 0;
	for (dit = tp.begin(); dit != tp.end(); dit++) {
		cout << pos << ".\t" << fd.getElement(*dit)->tostring() << endl;

		pos++;
	}
	cout << endl << endl;

	cout << "~! Caminho Critico ~" << endl; pos = 0; PathNode *cur;
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