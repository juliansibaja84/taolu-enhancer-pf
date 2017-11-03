#include "Classifier.h"


Classifier::Classifier()
{
	Classifier::line = NULL;
}


Classifier::~Classifier()
{
}

//----------------------------------------For Training----------------------------------------------------
//--------------------------------------------------------------------------------------------------------
int Classifier::doTraining()
{
	const char *error_msg;
	set_param();
	read_problem(input_file_name);
	error_msg = svm_check_parameter(&prob, &param);

	if (error_msg)
	{
		//fprintf(stderr,"ERROR: %s\n",error_msg);
		
	}

	if (cross_validation)
	{
		do_cross_validation();
	}
	else
	{
		model = svm_train(&prob, &param);
		if (svm_save_model(model_file_name, model))
		{
			//fprintf(stderr, "can't save model to file %s\n", model_file_name);
			//exit(1);
			return 1;
		}
		//svm_free_and_destroy_model(&model);
	}
	svm_destroy_param(&param);
	free(prob.y);
	free(prob.x);
	free(x_space);
	free(line);

	return 0;
}

int Classifier::read_problem(const char *filename)
{
	int max_index, inst_max_index, i;
	size_t elements, j;
	FILE *fp = fopen(filename, "r");
	char *endptr;
	char *idx, *val, *label;

	if (fp == NULL)
	{
		//fprintf(stderr,"can't open input file %s\n",filename);
		//exit(1);
	}

	prob.l = 0;
	elements = 0;
	max_line_len = 1024;
	line = Malloc(char, max_line_len);
	while (readline(fp) != NULL)
	{
		char *p = strtok(line, " \t"); // label

		// features
		while (1)
		{
			p = strtok(NULL, " \t");
			if (p == NULL || *p == '\n') // check '\n' as ' ' may be after the last feature
				break;
			++elements;
		}
		++elements;
		++prob.l;
	}
	rewind(fp);

	prob.y = Malloc(double, prob.l);
	prob.x = Malloc(struct svm_node *, prob.l);
	x_space = Malloc(struct svm_node, elements);

	max_index = 0;
	j = 0;
	for (i = 0; i<prob.l; i++)
	{
		inst_max_index = -1; // strtol gives 0 if wrong format, and precomputed kernel has <index> start from 0
		readline(fp);
		prob.x[i] = &x_space[j];
		label = strtok(line, " \t\n");
		if (label == NULL) { // empty line
			//exit_input_error(i + 1);
		}
		prob.y[i] = strtod(label, &endptr);
		if (endptr == label || *endptr != '\0') {
			//exit_input_error(i + 1);
		}
		while (1)
		{
			idx = strtok(NULL, ":");
			val = strtok(NULL, " \t");

			if (val == NULL)
				break;

			errno = 0;
			x_space[j].index = (int)strtol(idx, &endptr, 10);
			if (endptr == idx || errno != 0 || *endptr != '\0' || x_space[j].index <= inst_max_index){
				//exit_input_error(i + 1);
			}else{
				inst_max_index = x_space[j].index;
				}
			errno = 0;
			x_space[j].value = strtod(val, &endptr);
			if (endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr))) {
				//exit_input_error(i + 1);
			}
			++j;
		}

		if (inst_max_index > max_index)
			max_index = inst_max_index;
		x_space[j++].index = -1;
	}

	if (param.gamma == 0 && max_index > 0)
		param.gamma = 1.0 / max_index;

	if (param.kernel_type == PRECOMPUTED)
		for (i = 0; i<prob.l; i++)
		{
			if (prob.x[i][0].index != 0)
			{
				//fprintf(stderr,"Wrong input format: first column must be 0:sample_serial_number\n");
				//exit(1);
			}
			if ((int)prob.x[i][0].value <= 0 || (int)prob.x[i][0].value > max_index)
			{
				//fprintf(stderr,"Wrong input format: sample_serial_number out of range\n");
				//exit(1);
			}
		}

	fclose(fp);
	return prob.l;
}

char* Classifier::readline(FILE *input)
{
	int len;

	if (fgets(line, max_line_len, input) == NULL)
		return NULL;

	while (strrchr(line, '\n') == NULL)
	{
		max_line_len *= 2;
		line = (char *)realloc(line, max_line_len);
		len = (int)strlen(line);
		if (fgets(line + len, max_line_len - len, input) == NULL)
			break;
	}
	return line;
}

void Classifier::set_param()
{
	int i;
	//void(*print_func)(const char*) = NULL;	// default printing to stdout
											// default values
	param.svm_type = C_SVC;
	param.kernel_type = RBF;
	param.degree = 3;
	param.gamma = 0;	// 1/num_features
	param.coef0 = 0;
	param.nu = 0.5;
	param.cache_size = 100;
	param.C = 1;
	param.eps = 1e-3;
	param.p = 0.1;
	param.shrinking = 1;
	param.probability = 0;
	param.nr_weight = 0;
	param.weight_label = NULL;
	param.weight = NULL;
	cross_validation = 0;

	// parse options
	/*
	for (i = 1; i<argc; i++)
	{
		if (argv[i][0] != '-') break;
		if (++i >= argc)
			exit_with_help();
		switch (argv[i - 1][1])
		{
		case 's':
			param.svm_type = atoi(argv[i]);
			break;
		case 't':
			param.kernel_type = atoi(argv[i]);
			break;
		case 'd':
			param.degree = atoi(argv[i]);
			break;
		case 'g':
			param.gamma = atof(argv[i]);
			break;
		case 'r':
			param.coef0 = atof(argv[i]);
			break;
		case 'n':
			param.nu = atof(argv[i]);
			break;
		case 'm':
			param.cache_size = atof(argv[i]);
			break;
		case 'c':
			param.C = atof(argv[i]);
			break;
		case 'e':
			param.eps = atof(argv[i]);
			break;
		case 'p':
			param.p = atof(argv[i]);
			break;
		case 'h':
			param.shrinking = atoi(argv[i]);
			break;
		case 'b':
			param.probability = atoi(argv[i]);
			break;
		case 'q':
			print_func = &print_null;
			i--;
			break;
		case 'v':
			cross_validation = 1;
			nr_fold = atoi(argv[i]);
			if (nr_fold < 2)
			{
				fprintf(stderr, "n-fold cross validation: n must >= 2\n");
				exit_with_help();
			}
			break;
		case 'w':
			++param.nr_weight;
			param.weight_label = (int *)realloc(param.weight_label, sizeof(int)*param.nr_weight);
			param.weight = (double *)realloc(param.weight, sizeof(double)*param.nr_weight);
			param.weight_label[param.nr_weight - 1] = atoi(&argv[i - 1][2]);
			param.weight[param.nr_weight - 1] = atof(argv[i]);
			break;
		default:
			fprintf(stderr, "Unknown option: -%c\n", argv[i - 1][1]);
			exit_with_help();
		}
	}
	*/
	//svm_set_print_string_function(NULL);
	// determine filenames

	//if (i >= argc)
	//	exit_with_help();
}

void Classifier::do_cross_validation()
{
	int i;
	int total_correct = 0;
	double total_error = 0;
	double sumv = 0, sumy = 0, sumvv = 0, sumyy = 0, sumvy = 0;
	double *target = Malloc(double, prob.l);

	svm_cross_validation(&prob, &param, nr_fold, target);
	if (param.svm_type == EPSILON_SVR ||
		param.svm_type == NU_SVR)
	{
		for (i = 0; i<prob.l; i++)
		{
			double y = prob.y[i];
			double v = target[i];
			total_error += (v - y)*(v - y);
			sumv += v;
			sumy += y;
			sumvv += v*v;
			sumyy += y*y;
			sumvy += v*y;
		}

		//printf("Cross Validation Mean squared error = %g\n", total_error / prob.l);
		//printf("Cross Validation Squared correlation coefficient = %g\n",
		//	((prob.l*sumvy - sumv*sumy)*(prob.l*sumvy - sumv*sumy)) /
		//	((prob.l*sumvv - sumv*sumv)*(prob.l*sumyy - sumy*sumy))
		//);
		crossvaldata[0] = total_error / prob.l;
		crossvaldata[1] = ((prob.l*sumvy - sumv*sumy)*(prob.l*sumvy - sumv*sumy)) /
						((prob.l*sumvv - sumv*sumv)*(prob.l*sumyy - sumy*sumy));
	}
	else
	{
		for (i = 0; i<prob.l; i++)
			if (target[i] == prob.y[i])
				++total_correct;
		//printf("Cross Validation Accuracy = %g%%\n", 100.0*total_correct / prob.l);
		crossvaldata[0] = 100.0*total_correct / prob.l;
		crossvaldata[1] = 0;
	}
	free(target);
}

//----------------------------------------For Prediction--------------------------------------------------
//--------------------------------------------------------------------------------------------------------

double Classifier::doPrediction(double * angles, int predict_P)
{
	int i;
	predict_probability = predict_P;
	// parse options
	
	x = (struct svm_node *) malloc(max_nr_attr * sizeof(struct svm_node));
	model = svm_load_model(model_file_name);
	if (predict_probability)
	{
		if (svm_check_probability_model(model) == 0)
		{
			//fprintf(stderr, "Model does not support probabiliy estimates\n");
			//exit(1);
		}
	}
	else
	{
		if (svm_check_probability_model(model) != 0) {
			//info("Model supports probability estimates, but disabled in prediction.\n");
		}
	}
	//---------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------
	int correct = 0;
	int total = 0;
	double error = 0;
	double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;

	int svm_type = svm_get_svm_type(model);
	int nr_class = svm_get_nr_class(model);

	int j;

	if (predict_probability)
	{
		if (svm_type == NU_SVR || svm_type == EPSILON_SVR) {
			//info("Prob. model for test data: target value = predicted value + z,\nz: Laplace distribution e^(-|z|/sigma)/(2sigma),sigma=%g\n", svm_get_svr_probability(model));
		}
		else
		{
			int *labels = (int *)malloc(nr_class * sizeof(int));
			svm_get_labels(model, labels);
			prob_estimates = (double *)malloc(nr_class * sizeof(double));
			//fprintf(output, "labels");
			for (j = 0; j<nr_class; j++)
				//fprintf(output, " %d", labels[j]);
			//fprintf(output, "\n");
			free(labels);
		}
	}

	double predict_label;
	for (i = 0; i < 13; i++) {
		x[i].index = i+1;
		x[i].value = angles[i];
	}
	x[13].index = -1;

	if (predict_probability && (svm_type == C_SVC || svm_type == NU_SVC))
	{
		predict_label = svm_predict_probability(model, x, prob_estimates);
		//fprintf(output, "%g\n", predict_label);
		//fprintf(output, " %g", prob_estimates[j]);

	}
	else
	{
		predict_label = svm_predict(model, x);
		//fprintf(output, "%g\n", predict_label);
	}
	if (predict_probability)
		free(prob_estimates);



	//---------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------
	svm_free_and_destroy_model(&model);
	free(x);
	return predict_label;
}

