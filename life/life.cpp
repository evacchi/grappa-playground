#include <Grappa.hpp>
#include <limits>
#include <cassert>
#include <array>
#include <chrono>
#include <thread>

enum class State { Dead, Alive };


namespace Spaces {

    struct Range {
        long lower = 0, upper = 0;
        size_t width() { return upper-lower; }
        // Range() {}
        Range(long upper_) : upper(upper_) {
            // std::cout << "range created: " << width() << "\n";
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
            // std::cout << "domain created: " << x.width() << " x " << y.width() << "\n";
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

        Board(Domain domain_) : domain(domain_) , values(new State[domain_.size()]) {
            for (int i = 0; i < domain_.size(); i++) values[i] = State::Dead;
        }
        Board(Domain domain_, State* values_) : domain(domain_), values(values_) {}


        long project(long i, long j) {
            long result = domain.x.normalized(i) + domain.x.width() * domain.y.normalized(j);

            // std::cout << i << ", " << j << " -> " << result << "\n";

            return result;
        }

        State& get(long i, long j) {
            // std::cout << i << "," << j << "\n";
            // assert ( i >= domain.x.lower && i < domain.x.upper 
            //         && j >= domain.y.lower && j < domain.y.upper );

            // std::cout << i << ", " << j << " -> " 
            //           << ((values[ project(i,j) ] == State::Alive)? "alive":"dead") << "\n";

            return values[ project(i,j) ];
        }

        State& operator()(long i, long j) {
            return get(i,j);
        }

        // Board& slice(Range xx, Range yy) {
        //     Domain new_domain(xx, yy);
        //     Board new_board (new_domain);    

        //     // quick&dirty: this should become a memcpy!                        
        //     for (int i = new_board.domain.x.lower; i < domain.x.upper; i++) {
        //         for (int j = new_board.domain.y.lower; j < domain.y.upper; j++) {
        //             new_board(i,j) = get(i,j);
        //         }
        //     }

        //     return new_board;
        // }

        // Board& operator()(Range xx, Range yy) {
        //     return slice(xx,yy);
        // }

        // Board& operator()(Domain xy) {
        //     return slice(xy.x,xy.y);
        // }

        void assign(Board value) {
            // quick&dirty: this should become a memcpy!                        
            for (int i = value.domain.x.lower; i < value.domain.x.upper; i++) {
                for (int j = value.domain.y.lower; j < value.domain.y.upper; j++) {
                    get(i,j) = value.get(i,j);
                }
            }

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
        

    State& operator()(const size_t& x,  const size_t& y) {
        return board(x,y);
    }

    size_t count_alive(Spaces::Range x, Spaces::Range y) {
        size_t total = 0;
        for (int i = x.lower; i <= x.upper; i++) {
            for (int j = y.lower; j <= y.upper; j++) {
                if(board.get(i,j) == State::Alive) {
                    // std::cout << i << ", " << j << " is alive\n";
                    total++;  
                }

            }
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
                size_t num_alive = count_alive( Spaces::Range(i-1,i+1), Spaces::Range(j-1, j+1) );
                if (is_alive) num_alive--;
                // std::cout << i << ", " << j <<  ": num_alive = " << num_alive 
                //           << " => " << (( (2 == num_alive && is_alive) || num_alive == 3 ) ? "alive" : "dead") << "\n"; 
                temp_board(i,j) = ( (2 == num_alive && is_alive) || num_alive == 3 ) ? State::Alive : State::Dead ;
            }
        }

        board.assign(temp_board);

    } 

    void pretty_print() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << (char)27 << "[2J" << (char)(27) << "[;H";
        for (long i = 0; i < grid_domain.x.upper; i++) {
            for (long j = 0; j < grid_domain.y.upper; j++) {
                if (i == 0 || i == grid_domain.x.upper - 1) std::cout << "-";
                else if (j == 0 || j == grid_domain.y.upper - 1) std::cout << "|";
                else std::cout << ( board.get(i,j) == State::Alive? "#" : " " );
            }
            std::cout << "\n";
        }

    }
 


};

const int grid_height = 10;
const int grid_width = 10;


void main_body() {
    auto game = Game::Game(grid_height,grid_width);
    game(grid_height/2 + 1, grid_width/2    ) = State::Alive;
    game(grid_height/2 + 1, grid_width/2 + 1) = State::Alive;
    game(grid_height/2 + 1, grid_width/2 + 2) = State::Alive;
        
    game.pretty_print();

    for (int i = 0; i < 3; i++) {
        game.step();
        game.pretty_print();
    }

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
