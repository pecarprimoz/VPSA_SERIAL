/*
* 1. KORAK JE SOBEL
* 2. KORAK JE RAČUNANJE ENERGIJ, RAČUNANJE VSOT LAHKO PARALELIZIRAMO, RABIMO BARRIER
* MED VSAKO VRSTICO KI RAČUNAMO ENERGIJE
* 3. NE SPLAČA SE VEDNO NA NOVO KOPIRAT CELOTNE TABELE, POSKUSI VEČ ŠIVOV NA ENKRAT
*
*
*
*
*/

/*
* File:   main.c
* Author: primoz
*
* Created on December 16, 2017, 11:31 AM
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "pgm.h"
#include <limits.h>
#include <omp.h>
#define MAXITER 500
void seamCarving(int* slikaInput, int InputW, int InputH, int* siv);
void poisciSive(int h, int w, int* slikaInput, int* siv);
int getPixelCPU(PGMData *input, int y, int x)
{
	if (x < 0) x = 1;
	else if (x >= input->width) x = input->width - 1;
	if (y < 0) y = 1;
	else if (y >= input->height) y = input->height - 1;

	return input->image[x + y * input->width];
}

void sobelCPU(PGMData *input, PGMData *output)
{
	int i, j;
	int Gx, Gy;
	int tempPixel;

	//za vsak piksel v sliki
	for (i = 0; i<(input->height); i++)
		for (j = 0; j<(input->width); j++)
		{
			Gx = -getPixelCPU(input, i - 1, j - 1) - 2 * getPixelCPU(input, i - 1, j) -
				getPixelCPU(input, i - 1, j + 1) + getPixelCPU(input, i + 1, j - 1) +
				2 * getPixelCPU(input, i + 1, j) + getPixelCPU(input, i + 1, j + 1);
			Gy = -getPixelCPU(input, i - 1, j - 1) - 2 * getPixelCPU(input, i, j - 1) -
				getPixelCPU(input, i + 1, j - 1) + getPixelCPU(input, i - 1, j + 1) +
				2 * getPixelCPU(input, i, j + 1) + getPixelCPU(input, i + 1, j + 1);
			tempPixel = sqrt((float)(Gx*Gx + Gy * Gy));
			if (tempPixel>255)
				output->image[i*output->width + j] = 255;
			else
				output->image[i*output->width + j] = tempPixel;
		}
}

int returnMinimumValue(int a, int b, int c);
void printVectorAsMatrix(int s, int w, int v[]) {
	for (int i = 0; i<s; i++) {
		if (i%w == 0) {
			printf("\n");
		}
		printf(" %d ", v[i]);
	}
	printf("\n");
}
int returnMinimumValue(int a, int b, int c) {
	if (a<b && a<c) {
		return a;
	}
	else if (b<a && b<c) {
		return b;
	}
	return c;
}


int main()
{
	//Merjenje časa
	double start_time = omp_get_wtime();

	//Izvajanje Sobela nad neko novo sliko
	PGMData slikaInputP, slikaKoncna, slikaSobel;

	//Preberemo original sliko v slikaInputP
	readPGM("towerpgm.pgm", &slikaInputP);
	slikaSobel.width = slikaInputP.width;
	slikaSobel.height = slikaInputP.height;
	slikaSobel.max_gray = 255;
	slikaSobel.image = (int *)malloc(slikaInputP.height * slikaInputP.width * sizeof(int));

	slikaKoncna.image = (int *)malloc(slikaInputP.height * slikaInputP.width * sizeof(int));
	slikaKoncna.height = slikaInputP.height;
	slikaKoncna.width = slikaInputP.width;
	slikaKoncna.max_gray = slikaInputP.max_gray;

	int * siv = (int *)malloc(slikaInputP.height * sizeof(int));
	int st_iteracij = 0;

	//Število iteracij je definirano v DEFINE
	while (st_iteracij<MAXITER) {

		//Sobelu pošljemo original sliko in ven dobimo filter sobela
		sobelCPU(&slikaInputP, &slikaSobel);
		//Poračunamo kumulative
		
		seamCarving(slikaSobel.image, slikaInputP.width, slikaInputP.height, siv);
		
		slikaInputP.width--;

		//printf("%d %d\n", slikaInputP.width, slikaInputP.height);
		for (int i = 0; i < slikaInputP.height; i++) {
			for (int j = 0; j < slikaInputP.width; j++) {
				int curx = j;
				int cury = i;
				int tmpx = curx;
				int koncniInd;

				if (curx < slikaInputP.width && cury < slikaInputP.height) {
					koncniInd = cury * slikaInputP.width + curx;
					if (curx >= siv[cury]) {
						tmpx++;
					}
					slikaKoncna.image[koncniInd] = slikaInputP.image[cury*(slikaInputP.width + 1)+tmpx];
				}
			}
		}

		//writePGM("towerSobelDebug.pgm", &slikaInputP);

		slikaKoncna.width--;
		slikaSobel.width--;
		//printf("%d %d\n", slikaKoncna.width, slikaKoncna.height);

		int* temp = slikaInputP.image;
		slikaInputP.image = slikaKoncna.image;
		slikaKoncna.image = temp;
		
		st_iteracij++;
	}

	/*
	int max_value = 0;
	for (int i = 0; i<slikaSobel.width*slikaSobel.height; i++) {
	if (slikaSobel.image[i] > max_value){
	max_value = slikaSobel.image[i];
	}
	}
	for (int i = 0; i<slikaSobel.width*slikaSobel.height; i++) {
	slikaSobel.image[i] =(int)((float)slikaSobel.image[i] / max_value * 255);
	}
	*/
	writePGM("serial_300iterfix.pgm", &slikaInputP);
	printf("Čas izvajanja na CPU: %f\n", omp_get_wtime() - start_time);
	return 0;
}

void seamCarving(int * slikaInput, int InputW, int InputH, int * siv) {
	int w;
	int h;
	w = InputW;
	h = InputH;

	int left;
	int middle;
	int right;
	for (int i = h - 2; i >= 0; i--) {
		for (int j = 0; j < w; j++) {
			//gremo po celotni vrstici
			//trenutna pozicija glede na širino in stolpec v katerem smo plus trenutni indeks 
			int trenutna_pozicija_v_vektorju = (w*i) + j;
			int trenutna_vrednost_v_vektorju = slikaInput[trenutna_pozicija_v_vektorju];
			left = slikaInput[trenutna_pozicija_v_vektorju + (w - 1)];
			middle = slikaInput[trenutna_pozicija_v_vektorju + (w)];
			right = slikaInput[trenutna_pozicija_v_vektorju + (w + 1)];
			//ROBNI PRIMER 1, CE SEM NA ZACETKU NE PREVERJAM LEVEGA
			if (j == 0) {
				//ÈE JE MOJ SPODNJI SOSED DIREKTNO POD MANO MANJŠI OD MOJEGA DESNEGA SOSEDA GA PRIŠTEJEM
				if (middle < right) {
					trenutna_vrednost_v_vektorju += middle;
				}
				else {
					trenutna_vrednost_v_vektorju += right;
				}
			}
			//ROBNI PRIMER 2, CE SEM NA KONCU NE PREVERJAM DESNEGA
			else if (j == w - 1) {
				if (middle < left) {
					trenutna_vrednost_v_vektorju += middle;
				}
				else {
					trenutna_vrednost_v_vektorju += left;
				}
			}
			//NI ROBNEGA PRIMERA, GLEDAM 3 SPODNJE SOSEDE
			else {
				trenutna_vrednost_v_vektorju += returnMinimumValue(left, middle, right);
			}
			//Posodobimo trenutno sliko
			//printf("%d \n", trenutna_vrednost_v_vektorju);
			slikaInput[trenutna_pozicija_v_vektorju] = trenutna_vrednost_v_vektorju;
		}
	}
	poisciSive(h, w, slikaInput, siv);
}

void poisciSive(int h, int w, int* slikaInput, int* siv) {
	int najmanjsa_vrednost_v_vrstici = INT_MAX;
	int indeks_najmanjse_vrednosti_v_vrstici;
	for (int i = 0; i < w; i++) {
		if (slikaInput[i] < najmanjsa_vrednost_v_vrstici) {
			najmanjsa_vrednost_v_vrstici = slikaInput[i];
			indeks_najmanjse_vrednosti_v_vrstici = i;
		}
	}
	//Shranimo prvi siv iz prve vrstice
	siv[0] = indeks_najmanjse_vrednosti_v_vrstici;
	int siv_c = 1;
	int leviElement = INT_MAX;
	int desniElement = INT_MAX;
	while (siv_c < h) {
		//Gledamo 3 primere, levi desni in srednji
		int indeksNaslednjeVrstice = siv_c * w + indeks_najmanjse_vrednosti_v_vrstici;
		int elementNaslednjeVrstice = slikaInput[indeksNaslednjeVrstice];

		if (indeks_najmanjse_vrednosti_v_vrstici - 1 >= 0) {
			leviElement = slikaInput[indeksNaslednjeVrstice - 1];
		}

		if (indeks_najmanjse_vrednosti_v_vrstici + 1 < w) {
			desniElement = slikaInput[indeksNaslednjeVrstice + 1];
		}
		indeks_najmanjse_vrednosti_v_vrstici += leviElement < elementNaslednjeVrstice
			? (leviElement < desniElement ? -1 : 1) : (elementNaslednjeVrstice < desniElement ? 0 : 1);

		siv[siv_c] = indeks_najmanjse_vrednosti_v_vrstici;
		leviElement = desniElement = elementNaslednjeVrstice = INT_MAX;
		siv_c++;
	}
}
