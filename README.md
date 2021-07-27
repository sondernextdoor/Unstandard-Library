# Unstandard-Library
Reimplementation of std::vector and std::string without using the standard library.

There is zero reason to use this over the standard library; it was mainly written as a learning expierence. Some methods are faster than their std counterparts, while some are slower. The biggest differences can be found with the push_back() function (~5x faster than the standard libray) and the at() function (~2x faster).
