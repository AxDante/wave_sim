
#define _USE_MATH_DEFINES

#include <iostream>
#include <cstdio>
#include <ctime>
#include <math.h>
#include "WaveSystemParallelNew.h"
#include "Wall.h"

WaveSystemParallelNew::WaveSystemParallelNew(int row, int col, float mass, float step)
{
	part_mass = mass;
	columns = col;
	rows = row;
	timeStep = step;

	Initialize();

}

void WaveSystemParallelNew::Initialize() {


	sysCounter = 0.0f;
	part_size = 0.35;
	baseSpreadSpeed = 5;
	vector<Vector3f> startPosVel(numParticles * 2);
	//vector<Vector4f> springs;
	vector<Vector3f> m_face;

	vector< Vector3f > initPos;
	vector< Vector3f > initVel;
	vector< Vector3f > forcedPos;
	vector< Vector3f > forcedVel;
	vector< Vector3f > statePos;
	vector< Vector3f > stateVel;
	//vector< Vector3f > vibrationDir;
	//vector< float > vibrationValue;
	//vector< float > energyStored;

	switchBoarderReflection = false;
	switchWallReflection = false;

	m_vVecState.empty();

	const float r_struct = 0.7f;				// structural spring rest length
	const float r_init = 1.00f * r_struct;		// initial rest length

												//center.push_back((int)(row + 1) * (col) / 2 + col / 4);
												//center.push_back((int)(row + 1) * (col) / 2 + col / 8);
												//center.push_back((int)(row + 1) * (col) / 2 - col / 4);
												//center.push_back((int)(row + 1) * (col) / 2 - col / 8);

												//center.push_back((int) row * col / 2 + col / 4 + row * col / 4);
												//center.push_back((int) row * col * 5 / 6 + col / 4);
	
	for (int sc = 0; sc < 15; sc++) {
		center.push_back((int)rows * columns / 8 - 7 + sc);
		sourceType.push_back(1);
	}

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {

			initPos.push_back(Vector3f(0.5 * r_init * columns - r_init * j, 0, 0.5 * r_init * rows - r_init * i));
			initVel.push_back(Vector3f(0, 0, 0));

			statePos.push_back(Vector3f(r_init * j, 0, r_init * i));
			stateVel.push_back(Vector3f(0, 0, 0));

			Vector3f initState = Vector3f(0.5 * r_init * columns - r_init * j, 0, 0.5 * r_init * rows - r_init * i);
			this->m_vVecState.push_back(initState);
			this->m_vVecState.push_back(Vector3f(0, 0, 0));

			//energyStored.push_back(1.0f - abs(i - 0.5f*rows) / 20.0 - abs(j - 0.5f*columns) / 20.0);

			energyStored.push_back(0.0f);
			particleType.push_back(0);

			if (i < columns) {
				forcedPos.push_back(Vector3f(0, 0, 0));
				forcedVel.push_back(Vector3f(0, -1, 0));
			}
			else {
				forcedPos.push_back(Vector3f(-100, -100, -100));
				forcedVel.push_back(Vector3f(-100, -100, -100));
			}

		}
	}


	for (int source = 0; source < center.size(); source++) {
		vector<float > phasePush;
		vector<float > magnitudePush;
		vector<int > spreadCountPush;
		vector<int > sourceCheckPush;
		vector<int > toCalculatePush;
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < columns; j++) {
				if (i * columns + j == center[source]) {
					magnitudePush.push_back(1.00f);
					phasePush.push_back(0.001f);
				}
				else {
					magnitudePush.push_back(0.0f);
					phasePush.push_back(0.0f);
				}
				spreadCountPush.push_back(baseSpreadSpeed);
				sourceCheckPush.push_back(0);
			}
		}
		for (int ctr = 0; ctr < rows * columns; ctr++) {
			toCalculatePush.push_back(ctr);
		}
		sourceCounter.push_back(0.0);
		phaseStored.push_back(phasePush);
		magnitudeStored.push_back(magnitudePush);
		sourceChecked.push_back(sourceCheckPush);
		spreadCounter.push_back(spreadCountPush);
	}


	// SET UP WALLS
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {
			if (i > 0.6*rows && i < 0.63*rows && j > 0.5*columns + 5) {
				particleType[i * columns + j] = 1;
			}
			if (i > 0.6*rows && i < 0.63*rows && j < 0.5*columns - 5) {
				particleType[i * columns + j] = 1;
			}
			if (i > 0.6*rows && i < 0.63*rows && j < 0.5*columns + 5 && j > 0.5*columns - 5 ) {
				particleType[i * columns + j] = 1;
				//printf("sol%d ", i * columns + j);
			}
			if (i > 0.63*rows ) {
				for (int sc = 0; sc < 15; sc++) {
					sourceChecked[sc][i * columns + j] = 1;
				}
			}
			//printf("%d",sourceChecked[0][i * columns + j]);
		}
		//printf("\n");
	}


	const float k_spring_struct = 50;			//spring 
	const float k_spring_shear = 50;			// spring stiffness
	const float k_spring_flex = 50;				// spring stiffness
	const float r_shear = sqrt(2) * r_struct;	// shear spring rest length
	const float r_flex = 2.0f * r_struct;		// flexible spring rest length

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < columns; j++) {

			int index_I = index(i, j, rows, columns);
			int index_UL = index(i - 1, j + 1, rows, columns);
			int index_U = index(i, j + 1, rows, columns);
			int index_UR = index(i + 1, j + 1, rows, columns);
			int index_L = index(i - 1, j, rows, columns);
			int index_R = index(i + 1, j, rows, columns);
			int index_B = index(i, j - 1, rows, columns);
			int index_BL = index(i - 1, j - 1, rows, columns);
			int index_BR = index(i + 1, j - 1, rows, columns);

			int index_RR = index(i, j + 2, rows, columns);
			vector<int> toPush;
			toPush.push_back(index_I);
			toPush.push_back(index_UL);
			toPush.push_back(index_U);
			toPush.push_back(index_UR);
			toPush.push_back(index_L);
			toPush.push_back(index_R);
			toPush.push_back(index_BL);
			toPush.push_back(index_B);
			toPush.push_back(index_BR);
			springs.push_back(toPush);
		}
	}

}

void WaveSystemParallelNew::dragMotion() {
	return;
}



void WaveSystemParallelNew::takeTimeStep()
{
	vector<Vector3f> f;
	vector< float > energy = getEnergy();
	const float k_drag = 0.001;			// drag constant

	vector<Vector3f > nState = getState();

	for (int i = 0; i < nState.size() / 2; i++) {
		for (int source = 0; source < center.size(); source++) {
			if (phaseStored[source][i] != 0 && sourceChecked[source][i] == 0) {
				for (int j = 0; j < this->springs.size(); j++) {
					if (this->springs[j][0] == i) {
						if (spreadCounter[source][i] == 0) {
							bool newSource = false;
							bool isSource = false;
							int sequence[9] = { 0, 2, 4, 5, 7, 1, 3, 6, 8 };
							for (int seq = 1; seq < 5; seq++) {
								int k = sequence[seq];
								
									int thisParticle = springs[j][0];
									int nextParticle = springs[j][k];
									float disToThis = (nState[center[source] * 2] - nState[thisParticle * 2]).abs();
									float disToNext = (nState[center[source] * 2] - nState[nextParticle * 2]).abs();
									float magnitudeNew = magnitudeStored[source][center[source]] * exp(-disToThis * waveDecayRate);
									//if (disToNext <= sourceCounter[source]) {
									//	printf("%.2f sc: ", sourceCounter[source]);
									if (magnitudeStored[source][i] > magnitudeReflectThreshold) {
										if (nextParticle == -1) {
											springs[j][k] = -2;  // Prevent multiple calculation on the same obstacle and increase calculation speed
											if (switchBoarderReflection) newSource = true;
										}
										else if (particleType[nextParticle] == 1) {
											springs[j][k] = -3;
											if (i == 2245) {
												newSource = true;
												sourceType.push_back(0);
											}
											if (i == 2255) {
												newSource = true;
												sourceType.push_back(0);
											}
											if (switchWallReflection) newSource = true;
										}
									}
									for (int sc = 0; sc < center.size(); sc++) {
										if (center[sc] == i) {
											isSource = true;
											//magnitudeStored[source][i] = magnitudeStored[source][center[source]] * exp(-disToNext * waveDecayRate);
											//phaseStored[source][i] = disToNext;
										}
									}
									//}
									//else {
										//sourceCounter[source] += 1;
									//}
								
							}
							if (!newSource || isSource ){
								for (int seq = 1; seq < 5; seq++) {
									//printf("%d", sourceType[source]);
									//bool foundNext = false;
									int k = sequence[seq];
									if (sourceType[source] == 1 && k == 5 || sourceType[source] == 0) {
										int thisParticle = springs[j][0];
										int nextParticle = springs[j][k];
										float disToThis = (nState[center[source] * 2] - nState[thisParticle * 2]).abs();
										float disToNext = (nState[center[source] * 2] - nState[nextParticle * 2]).abs();
										//if (disToNext <= sourceCounter[source]) {
										//	foundNext = true;
											//printf("%.2f sc: ", sourceCounter[source]);
										if (nextParticle != -1) { //&& sourceChecked[source][nextParticle] != 1) {
											if (phaseStored[source][nextParticle] < phaseStored[source][thisParticle]) {
												magnitudeStored[source][nextParticle] = magnitudeStored[source][center[source]] * exp(-disToNext * waveDecayRate);
												phaseStored[source][nextParticle] = disToNext;
												spreadCounter[source][i] = baseSpreadSpeed;
												sourceChecked[source][i] = 1;
											}
										}
										//}else{
										//	sourceCounter[source] += 0.1;
										//	//printf("%.2f sc: ", sourceCounter[source]);
										//}
									}
								}
							}
							else {
								if (center.size() < maxCenter) {
									center.push_back(i);
									vector<float > phasePush;
									vector<float > magnitudePush;
									vector<int > spreadCountPush;
									vector<int > sourceCheckPush;
									vector<int > toCalculatePush;
									for (int r = 0; r < rows; r++) {
										for (int c = 0; c < columns; c++) {
											if (r * columns + c == i) {
												magnitudePush.push_back(magnitudeStored[source][i]);
												phasePush.push_back(phaseStored[source][i]);
											}
											else {
												magnitudePush.push_back(0.0f);
												phasePush.push_back(0.0f);
											}
											spreadCountPush.push_back(baseSpreadSpeed);
											sourceCheckPush.push_back(0);
										}
									}
									for (int ctr = 0; ctr < rows * columns; ctr++) {
										toCalculatePush.push_back(ctr);
									}
									sourceCounter.push_back(0.0);
									phaseStored.push_back(phasePush);
									magnitudeStored.push_back(magnitudePush);
									sourceChecked.push_back(sourceCheckPush);
									spreadCounter.push_back(spreadCountPush);
								}
							}
						}
						else {
							spreadCounter[source][i] -= 1;
						}
					}
				}
			}
		}

		float totY = 0;
		for (int source = 0; source < center.size(); source++) {
			totY += magnitudeStored[source][i] * sin(2500 * sysCounter - phaseStored[source][i]);
		}
		nState[2 * i].y() = totY;// + totY2;

		f.push_back(nState[2 * i]);
		f.push_back(nState[2 * i + 1]);
	}

	sysCounter += timeStep;
	this->setState(f);
}

void WaveSystemParallelNew::draw()
{
	
	vector<Vector3f> state = getState();
	vector< float > energy = getEnergy();

	//if (count_wire == 0 || count_wire == 2 || count_wire == 3 || count_wire == 4) {
		for (int i = 0; i < state.size() / 2; i++) {
			Vector3f pos = state[i * 2];
			if (particleType[i] == 0) {

				GLfloat particleColor[] = { 0.5f, 1.0f * (0.5 * pos[1]  + 0.5), 0.5f, 1.0f };
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, particleColor);

			}else if(particleType[i] == 1){
				GLfloat particleColor[] = { 0.7f, 0.7f, 0.0f , 1.0f};
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, particleColor);
			}
			glPushMatrix();
			glTranslatef(pos[0], pos[1], pos[2]);
			glutSolidSphere(part_size + 0.2*energy[i], 10.0f, 10.0f);
			glPopMatrix();

		}
	//}

}

int WaveSystemParallelNew::index(int cur_row, int cur_col, int tot_row, int tot_col) {
	if (cur_row >= tot_row || cur_col >= tot_col || cur_row < 0 || cur_col < 0) {
		return -1;
	}
	else {
		return cur_row * tot_col + cur_col;
	}
}


void WaveSystemParallelNew::takeStep(float stepSize)
{
	vector<Vector3f> currentState = this->getState();
	vector<Vector3f> f0 = this->evalF(currentState);
	vector<Vector3f> f0_state;
	
	for (int i = 0; i < f0.size(); i++) {
		f0_state.push_back(currentState[i] + stepSize * f0[i]);
	}

	vector<Vector3f> f1 = this->evalF(f0_state);
	vector<Vector3f> nextState;

	for (int i = 0; i < f0.size(); i++) {
		nextState.push_back(currentState[i] + 0.5 * stepSize * (f0[i] + f1[i]));
	}

	this->setState(nextState);
}