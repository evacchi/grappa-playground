#include <Grappa.hpp>
#include <limits>
#include <cassert>
#include <array>

enum class State { Dead, Alive };


namespace Spaces {

    struct Range {
        long lower = 0, upper = 0;
        size_t width() { return upper-lower; }
        // Range() {}
        Range(long upper_) : upper(upper_) {
            std::cout << "created: " << width() << "\n";
        }
        Range(long lower_, long upper_) : lower(lower_), upper(upper_) {}
        Range(const Spaces::Range& r): lower(r.lower), upper(r.upper) {}
        size_t normalized(long i) {
            return i - lower;
        }

    };


    struct Domain {
        Range x, y;
        Domain(const Domain& d) : x(d.x), y(d.y) {}
        Domain(Range x_, Range y_)  : x(x_), y(y_) {
            std::cout << "created: " << x.width() << " x " << y.width() << "\n";
        }
        size_t size() {
            return x.width() * y.width();
        }
    };


    struct Subdomain : public Domain {
        
        Subdomain(Domain domain_) :  Domain::Domain(domain_)  {}
        
        void expand(long amount) {
            x.lower -= amount; 
            y.upper += amount;
        }
    };

    struct Board {
        Domain domain;
        State* values;

        Board(Domain domain_) : domain(domain_) , values(new State[domain_.size()]) {}
        Board(Domain domain_, State* values_) : domain(domain_), values(values_) {}

        State& get(long i, long j) {
            // std::cout << i << "," << j << "\n";
            // assert ( i >= domain.x.lower && i < domain.x.upper 
            //         && j >= domain.y.lower && j < domain.y.upper );
            return values[ domain.x.normalized(i) + domain.x.width() * domain.y.normalized(j) ];
        }

        State& operator()(long i, long j) {
            return get(i,j);
        }

        Board& slice(Range xx, Range yy) {
            Domain new_domain(xx, yy);
            Board new_board (new_domain);    

            // quick&dirty: this should become a memcpy!                        
            for (int i = new_board.domain.x.lower; i < domain.x.upper; i++) {
                for (int j = new_board.domain.y.lower; j < domain.y.upper; j++) {
                    new_board(i,j) = get(i,j);
                }
            }

            return new_board;
        }

        Board& operator()(Range xx, Range yy) {
            return slice(xx,yy);
        }

        Board& operator()(Domain xy) {
            return slice(xy.x,xy.y);
        }


    };

}


class Game {
    Spaces::Domain grid_domain;
    Spaces::Subdomain compute_domain;
    Spaces::Board board;

public:
    Game(size_t height, size_t width) :
        grid_domain({ 
            Spaces::Range(height + 2),
            Spaces::Range(width + 2)
        }),

        compute_domain(grid_domain),

        board(grid_domain) { }
        

    /*
      proc step(){
     
        var tempGrid: [this.computeDomain] State;
        forall (i,j) in this.computeDomain {
          var isAlive = this.grid[i,j] == State.alive;
          var numAlive = (+ reduce this.grid[ i-1..i+1, j-1..j+1 ]) - if isAlive then 1 else 0;      
          tempGrid[i,j] = if ( (2 == numAlive && isAlive) || numAlive == 3 ) then State.alive else State.dead ;
        }
     
        this.grid[this.computeDomain] = tempGrid;
     
      }
    */

    State& operator()(const size_t& x,  const size_t& y) {
        return board(x,y);
    }

    size_t count_alive(Spaces::Board b) {
        size_t total = 0;
        for (int i = 0; i < b.domain.size(); i++) {
            if(b.values[i] == State::Alive) total++;  
        }
        return total;
    }


    void step() {
        Spaces::Board temp_board(compute_domain);
        // forall (size_t i, size_t j) {
        size_t i,j;
        for (long i = 0; i < grid_domain.x.upper; i++) {
            for (long j = 0; j < grid_domain.y.upper; j++) {
                bool is_alive = board.get(i,j) == State::Alive;
                size_t num_alive = count_alive( board.slice( Spaces::Range(i-1,i+1), Spaces::Range(j-1, j+1) ) ) - (is_alive? 1 : 0);
                temp_board(i,j) = ( (2 == num_alive && is_alive) || num_alive == 3 ) ? State::Alive : State::Dead ;
            }
        }

        board(compute_domain) = temp_board;

    } 

    void pretty_print(Spaces::Domain grid_domain, Spaces::Board board) {
        for (long i = 0; i < grid_domain.x.upper; i++) {
            for (long j = 0; j < grid_domain.y.upper; j++) {
                std::cout << ( board.get(i,j) == State::Alive? "#" : " " );
            }
            std::cout << "\n";
        }
    }
 


};

const int grid_height = 3;
const int grid_width = 3;


void main_body() {
    auto game = Game::Game(grid_height,grid_width);
    game(grid_height/2 + 1, grid_width/2    ) = State::Alive;
    game(grid_height/2 + 1, grid_width/2 + 1) = State::Alive;
    game(grid_height/2 + 1, grid_width/2 + 2) = State::Alive;

    game.step();
}

int main(int argc, char* argv[])
{
            main_body();

    // Grappa::init(&argc, &argv);
    // Grappa::run([]{
    //     main_body();
    // });
    // Grappa::finalize();
    return 0;
}
