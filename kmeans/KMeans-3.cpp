#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <GlobalHashMap.hpp>
#include <GlobalHashSet.hpp>
#include <limits>
#include <cassert>
#include <chrono>

#include "KMeans.hpp"

typedef GlobalAddress<int> GInt;
using namespace Grappa;

/**
 * Wierd name but essential function; decides witch centroid is closer to the given point
 * @param point point given
 * @param centroids pointer to centroids array
 * @param num_centroids number of centroids to check
 * @return closest centroid's index in centroids array(2nd param)
 */
int closest(Point point, Point* centroids, int num_centroids)
{
    int _closest=0;
    double min_d=dist(point,centroids[0]);
    
    for(int i=1;i<num_centroids;i++)
    {   
        int d = dist(point,centroids[i]);
        if(min_d >= d)
        {
            _closest=i;
            min_d=d;
        }
    }
    return _closest;
}
/**
 * Cumulative function that must be called after the closest centroid for each point is found
 * Calculates new centroids as describen in kmeans algorithm
 * @param points array of points
 * @param data array of cluster assignments
 * @param centroids return array of centroids
 * @param num_clusters number of clusters(so number of centroids)
 * @param num_points number of points 
 */

//          update_centroids(gpoints, gclusters, gcentroids, num_clusters, num_points);

void update_centroids(GPoint points, GInt clusters, GPoint centroids, int num_clusters, int num_points)
{
    Point new_centroids[num_clusters];
    int   population   [num_clusters];

    // for all cluster assignments
    // count the points in that cluster 
    forall(0, num_clusters, [&](int64_t i) {
        int k = delegate::read(clusters + i);
        population[k]++;

        Point p = delegate::read(points+i);
        new_centroids[k] += p;

    });
    forall(0, num_clusters, [&](int64_t i) {
        if(population[i] != 0)
            new_centroids[i] /= population[i];
    });
    forall<async>(0, num_clusters, [&](int64_t i) {
        delegate::write(centroids+i, new_centroids[i]);
    });
}

/**
 * main function
 * divided to two brances for master & slave processors respectively
 * @param argc commandline argument count
 * @param argv array of commandline arguments
 * @return 0 if success
 */
int main(int argc, char* argv[])
{
    int num_clusters = 10;
    int num_points   = 10000;
    int niters = 15;

    int job_size = num_points / Grappa::cores();

        
    Grappa::init(&argc, &argv);
    Grappa::run([=] {  

        GPoint gcentroids = global_alloc<Point>(num_clusters);
        GPoint gpoints    = global_alloc<Point>(num_points);
        GInt   gclusters  = global_alloc<int>(num_points);
        
        read_points(gpoints, gcentroids, num_points, num_clusters);
        
        //Main job of master processor is done here     
        for(int iter = 0; iter < niters; iter++)
        {   
            on_all_cores([=]{

                int64_t my_offset = job_size * Grappa::mycore();
                size_t my_job_size = job_size;
                if (Grappa::mycore() == Grappa::cores() - 1)
                    my_job_size += num_points % Grappa::cores();

                Point points[my_job_size];
                Point centroids[num_clusters];
                
                // fetch points from gmemory
                // finish([=]{
                    forall(0, my_job_size, [=](int64_t i) {
                        centroids[i] = delegate::read(gcentroids+my_offset+i);
                    });

                    forall(0, my_job_size, [=](int64_t i) {
                        points[i] = delegate::read(gpoints + my_offset + i);
                    });
                // });

                // compute & write back gclusters to gmemory
                forall(my_offset, my_job_size, [=](int64_t j) {
                    delegate::write(gclusters + j, my_offset + closest(points, centroids, num_clusters));
                });
            });
            
            
            update_centroids(gpoints, gclusters, gcentroids, num_clusters, num_points);

        }
        
    }
    Grappa::finalize();
    return 0;
}
/* EOF */
