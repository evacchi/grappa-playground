#include <Grappa.hpp>
#include <limits>
#include <cassert>
#include <array>
#include <chrono>
#include <thread>

enum class State { Dead, Alive };
typedef GlobalAddress<State> GState;

Grappa::GlobalCompletionEvent step_gce;

namespace Spaces {

    struct Range {
        long lower = 0, upper = 0;
        size_t width() const { return upper-lower; }
        Range() {}
        Range(long upper_) : upper(upper_) {}
        Range(long lower_, long upper_) : lower(lower_), upper(upper_) {}
        Range(const Spaces::Range& r): lower(r.lower), upper(r.upper) {}
        long indexof(long i) const { return i - lower; }

    };


    struct Domain {
        Range x, y;
        Domain(){}
        Domain(const Domain& d) : x(d.x), y(d.y) {}
        Domain(Range x_, Range y_)  : x(x_), y(y_) {}
        size_t size() { return x.width() * y.width(); }

        long project(long i, long j) const {
            long pos = x.indexof(i) + x.width() * y.indexof(j);
            return pos;
        }

        std::pair<long,long> unproject(long proj) const {
            long i,j;

            i = proj % x.width() + x.lower;
            j = proj / y.width() + y.lower;

            return std::make_pair(i,j);
        } 
    };

    struct Subdomain : public Domain {
        Subdomain(){}
        Subdomain(Domain domain_) :  Domain::Domain(domain_)  {}
        void expand(long amount) {
            x.lower -= amount; 
            y.upper += amount;
        }
    };

    struct Board {
        Domain domain;
        // State* values;
        GState values;



        struct Cell {
            Board* parent;
            long i,j;
            Cell(Board* parent_, long i_, long j_) : 
                parent(parent_),
                i(i_),
                j(j_) {}

            Board& operator=(const State& s) {
                Grappa::delegate::write(parent->get(i,j), s);
                return *parent;
            }
        };

        Board() {}

        Board(Domain domain_) : 
            domain(domain_) 
            // values(new State[domain_.size()]) 
        {
            // for (int i = 0; i < domain_.size(); i++) values[i] = State::Dead;
            values = Grappa::global_alloc<State>(domain_.size());
            Grappa::forall(values, domain_.size(), [] (State& s) { s = State::Dead; });
        }
        // Board(Domain domain_, State* values_) : domain(domain_), values(values_) {}



        GState get(long i, long j) const {
            return values + domain.project(i,j);
        }

        Cell operator()(long i, long j) {
            return Cell(this, i, j);
        }

        template<typename F>
        void forall_cells(F loop_body) {
            const Domain d = domain;
            Grappa::forall(values, domain.size(), [d, loop_body](int64_t pos, State& s) {
                std::pair<long,long> coords = d.unproject((long)pos);
                loop_body(coords, s);
            });
        }

        Board& assign(Board& value) {
            // quick&dirty: this should become a memcpy!                        
            // for (int i = value.domain.x.lower; i < value.domain.x.upper; i++) {
            //     for (int j = value.domain.y.lower; j < value.domain.y.upper; j++) {
            //         get(i,j) = value.get(i,j);
            //     }
            // }
            forall_cells([value](std::pair<long, long> coords, State& target){
                target = Grappa::delegate::read(value.get(coords.first,coords.second));
            });
            return *this;
        }

        Board& operator&=(Board value) {
            return assign(value);
        }


    };


}

    using namespace Grappa;


const int grid_height = 3;
const int grid_width  = 3;
const int iterations  = 10;

using namespace Spaces;

    // init_game(grid_height, grid_width);

Domain  grid_domain(
        Spaces::Range(grid_height + 2),
        Spaces::Range(grid_width + 2)
);

Subdomain  compute_domain(grid_domain);

Board     board;

void init_game() {
    

    Board _board(grid_domain);

    on_all_cores([=]{
        board = _board;
    });

}
  
size_t count_alive(Spaces::Range x, Spaces::Range y) {
    size_t total = 0;
    // for (int i = x.lower; i <= x.upper; i++) {
    //     for (int j = y.lower; j <= y.upper; j++) {
    //         if(board.get(i,j) == State::Alive) {
    //             // std::cout << i << ", " << j << " is alive\n";
    //             total++;  
    //         }

    //     }
    // }

    std::cout << "counting...";

    GlobalAddress<size_t> counter = global_alloc<size_t>(1);
    delegate::write(counter, 0);

    // we know that it's row-major
    for(long j = 0; j < y.width(); j++){
        Grappa::forall<async>(
            // first upper right cell
            board.get(x.lower,j),
            // for as many cells are in the horizontal range
            x.width(),
            [counter](State& v){
                if (v == State::Alive) 
                    delegate::increment<async>(counter,1);
            });
    };

    total = delegate::read(counter);
    global_free(counter);
    return total;
}
  

// Spaces::Board::Cell operator()(const size_t& x,  const size_t& y) {
//     return board(x,y);
// }


void step() {
    Spaces::Board temp_board(compute_domain);
    // size_t i,j;
    // for (long i = 0; i < grid_domain.x.upper; i++) {
    //     for (long j = 0; j < grid_domain.y.upper; j++) {
    board.forall_cells([temp_board](std::pair<long, long> coords, State& value){
        long i,j;
        std::tie(i,j) = coords; 
        bool is_alive = value == State::Alive;
        size_t num_alive = count_alive(Spaces::Range(i-1,i+1), Spaces::Range(j-1, j+1) );
        delegate::write<async>(
            temp_board.get(i,j), 
            ( (2 == num_alive && is_alive) || num_alive == 3 ) ? State::Alive : State::Dead 
        );
    });

    //         bool is_alive = board.get(i,j) == State::Alive;
    //         size_t num_alive = count_alive( Spaces::Range(i-1,i+1), Spaces::Range(j-1, j+1) );
    //         if (is_alive) num_alive--;
    //         // std::cout << i << ", " << j <<  ": num_alive = " << num_alive 
    //         //           << " => " << (( (2 == num_alive && is_alive) || num_alive == 3 ) ? "alive" : "dead") << "\n"; 
    //         temp_board(i,j) = ( (2 == num_alive && is_alive) || num_alive == 3 ) ? State::Alive : State::Dead ;
    //     }
    // }

    board &= temp_board;

} 

void pretty_print() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << (char)27 << "[2J" << (char)(27) << "[;H";
    for (long i = 0; i < grid_domain.x.upper; i++) {
        for (long j = 0; j < grid_domain.y.upper; j++) {
            if (i == 0 || i == grid_domain.x.upper - 1) std::cout << "-";
            else if (j == 0 || j == grid_domain.y.upper - 1) std::cout << "|";
            else std::cout << ( delegate::read(board.get(i,j)) == State::Alive? "#" : " " );
        }
        std::cout << "\n";
    }

}
 



void main_body() {
    init_game();
    board(grid_height/2 + 1, grid_width/2    ) = State::Alive;
    board(grid_height/2 + 1, grid_width/2 + 1) = State::Alive;
    board(grid_height/2 + 1, grid_width/2 + 2) = State::Alive;
    //game(grid_height/2, grid_width/2) = State::Alive;
    //game(grid_height/2-1, grid_width/2-1) = State::Alive;
    //game(grid_height/2+1, grid_width/2+1) = State::Alive;
        
    pretty_print();

    for (int i = 0; i < iterations; i++) {
        step();
        pretty_print();
        std::cout << i;
    }

}

int main(int argc, char* argv[])
{

    Grappa::init(&argc, &argv);
    Grappa::run([]{
        main_body();
    });
    Grappa::finalize();
    return 0;
}
