/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 

  // set the number of particles
  num_particles = 100;

  //Set standard deviations for x, y, and psi.
  double std_x = std[0];
  double std_y = std[1];
  double std_theta = std[2];

  //normal (Gaussian) distribution for x, y, and psi.
  default_random_engine gen;
  normal_distribution<double> dist_x(x, std_x);
  normal_distribution<double> dist_y(y,std_y);
  normal_distribution<double> dist_theta(theta,std_theta);

  Particle myParticle;
  double init_weight = 1.0/num_particles;
  for (int i = 0; i < num_particles; i++) {
    myParticle.x = dist_x(gen);
    myParticle.y = dist_y(gen);
    myParticle.theta = dist_theta(gen);
    myParticle.weight = init_weight;  
    particles.push_back(myParticle);
    weights.push_back(init_weight);
  }

  //reserve size for weights
  cout <<"Initialized" << endl;
  is_initialized = true;
  return;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/
  default_random_engine gen;
  //Set standard deviations for x, y, and psi.
  double std_x = std_pos[0];
  double std_y = std_pos[1];
  double std_theta = std_pos[2];

  //normal (Gaussian) distribution for x, y, and psi.
  normal_distribution<double> dist_x(0, std_x);
  normal_distribution<double> dist_y(0,std_y);
  normal_distribution<double> dist_theta(0,std_theta);
  //for each particle
  for (int i = 0; i < num_particles; ++i) {
    ////////////////////////////////////////////////////
    // Propogate position based on velocity and yaw_rate
    ////////////////////////////////////////////////////
    //avoid division by zero when integrating state. chose correct integration formula
    
    if (yaw_rate<0.0001){
        particles[i].x = particles[i].x + velocity*cos(particles[i].theta)*delta_t;
        particles[i].y = particles[i].y + velocity*sin(particles[i].theta)*delta_t;
    }
    else{
        particles[i].x = particles[i].x + velocity/yaw_rate*
        (sin(particles[i].theta + yaw_rate*delta_t) - sin(particles[i].theta));
        particles[i].y = particles[i].y + velocity/yaw_rate*
        (-cos(particles[i].theta + yaw_rate*delta_t) + cos(particles[i].theta));
    }
    //third state has no divide by zero worries
    particles[i].theta = particles[i].theta + yaw_rate*delta_t;


    /////////////////////////////////
    //Add gaussian noise to particle:
    /////////////////////////////////


    particles[i].x += dist_x(gen);
    particles[i].y += dist_y(gen);
    particles[i].theta += dist_theta(gen); 
    
  }

}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.
    for (int i =0; i < observations.size(); i++){

      observations[i].id = predicted[0].id;
      double best_dist_squared = pow(observations[i].x - predicted[0].x,2) + 
                                 pow(observations[i].y - predicted[0].y,2);

      for (int j = 1; j < predicted.size(); j++){
        double dist_squared = pow(observations[i].x - predicted[j].x,2) + 
                              pow(observations[i].y - predicted[j].y,2);
        if (dist_squared < best_dist_squared){
          observations[i].id = predicted[j].id;
          best_dist_squared = dist_squared;
        }
      }
    }

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		std::vector<LandmarkObs> observations, Map map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html

  // All calculations will be performed in maps coordinate system, 
  std::vector<LandmarkObs> observations_map;
  observations_map = observations; //copy to get size and ids. values replaced below
  double sensor_range_sqrd = pow(sensor_range,2); 

    for (int i = 0; i < num_particles; i++) {
      //First find all landmarks that are within range
      std::vector<LandmarkObs> landmarks_in_range;
      for (int j = 0; j < map_landmarks.landmark_list.size(); j++){
        double dist_squared = pow(particles[i].x - map_landmarks.landmark_list[j].x_f,2) + 
                              pow(particles[i].y - map_landmarks.landmark_list[j].y_f,2);
        if (dist_squared < sensor_range_sqrd){
          LandmarkObs myLandmark;
          myLandmark.id = j;
          myLandmark.x = map_landmarks.landmark_list[j].x_f;
          myLandmark.y = map_landmarks.landmark_list[j].y_f;
          landmarks_in_range.push_back(myLandmark);
        }
      }
      if (landmarks_in_range.size()>0){
        // now transform observations to map frame (to match landmarks_in_range coordinate system)
        // assuming the observations were made from the i'th particle's perspective
        std::vector<LandmarkObs> observations_map(observations.size());  
        for (int obs_i = 0; obs_i < observations.size(); obs_i++){
          observations_map[obs_i].x = cos(particles[i].theta)*observations[obs_i].x -
                                      sin(particles[i].theta)*observations[obs_i].y + 
                                      particles[i].x;
                                      

          observations_map[obs_i].y = sin(particles[i].theta)*observations[obs_i].x + 
                                      cos(particles[i].theta)*observations[obs_i].y + 
                                      particles[i].y;
        }


        //get associations
        dataAssociation(landmarks_in_range, observations_map);
        //update weights
        double w = 1.0;
        double prob_const = 1.0/2.0/M_PI/std_landmark[0]/std_landmark[1];
        for (int obs_i = 0; obs_i < observations_map.size(); obs_i++){
          int landmark_i = observations_map[obs_i].id;
          double x = observations_map[obs_i].x;
          double y = observations_map[obs_i].y;
          double ux = map_landmarks.landmark_list[landmark_i].x_f;
          double uy = map_landmarks.landmark_list[landmark_i].y_f;
          w*=prob_const * exp(-(pow((x - ux)/std_landmark[0],2)/2.0 + 
                                pow((y - uy)/std_landmark[1],2)/2.0));
        }
        particles[i].weight = w;
      }
      else{
        particles[i].weight = 0;
      }
      
      
      




    }

    //now normalize
    double scale_factor = 0.0;
    for (int i = 0; i < num_particles; i++){
      scale_factor+=particles[i].weight;

    }
    // cout << scale_factor << endl;
    for(int i = 0; i < num_particles; i++){
      particles[i].weight = particles[i].weight/scale_factor;
      weights[i] = particles[i].weight;
    }




}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution

  //preallocate space for new particles
  std::vector<Particle> particles_new(num_particles);
  particles_new.reserve(num_particles);
  //make random number generator
  std::default_random_engine generator;
  std::discrete_distribution<int> distribution(weights.begin(),weights.end());

  
  for (int i =0; i<num_particles; i++){
    int number = distribution(generator);
    particles_new[i] = particles[number]; 
  }
  particles = particles_new;

}

Particle ParticleFilter::SetAssociations(Particle particle, std::vector<int> associations, std::vector<double> sense_x, std::vector<double> sense_y)
{
	//particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
	// associations: The landmark id that goes along with each listed association
	// sense_x: the associations x mapping already converted to world coordinates
	// sense_y: the associations y mapping already converted to world coordinates

	//Clear the previous associations
	particle.associations.clear();
	particle.sense_x.clear();
	particle.sense_y.clear();

	particle.associations= associations;
 	particle.sense_x = sense_x;
 	particle.sense_y = sense_y;

 	return particle;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
