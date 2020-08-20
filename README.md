# OneSteinerTreeProblem
Implementation of (most of) Georgakopolous and Papadimitriou's O(n^2) 1-Steiner tree algorithm: <br/>
@article{DBLP:journals/jal/GeorgakopoulosP87,
  author    = {George K. Georgakopoulos and
               Christos H. Papadimitriou},
  title     = {The 1-Steiner Tree Problem},
  journal   = {J. Algorithms},
  volume    = {8},
  number    = {1},
  pages     = {122--130},
  year      = {1987}
}

Used CGAL 4.11.2 for everything, e.g., OODC/OOVD arrangement creation. 
Not sure how CGAL does some things and it's been a while since I've looked at the code, but I think this is at least an \Omega(n^2)log(n) implementation.
Had this code a while before I uploaded it.

I remember that I didn't implement their n^2 pre-processing to enable constant-time updates of MST length, but as will appear in 
"On the Restricted 1-Steiner Tree Problem" by P. Bose, A. D'Angelo, and S. Durocher (of which I'm a co-author), 
we _could_ accomplish constant-time updates with nlog n preprocessing (using a result from "Approximating geometric bottleneck shortest paths" by Bose et al. 2004).
I didn't implement that either though. It's possible I re-compute the MST every time which would give \Omega(n^3)log(n) time, but I think 
I probably used some kind of dynamic Delaunay allowing for log n updates.

I think I used the correct CGAL data types/primitives.
To be honest, as I'm not a mathematician, I found them confusing. I'm sure they're powerful though!
I enabled lazy computations which helps keep it from spinning its wheels. 
When always using the exact computations, it would sometimes run for far too long and sometimes would crash (by using up all its memory as far as I could tell).
My tests usually return quickly up to 6 or 7 input points, after which it takes far too long. 
This might be because in my application (i.e. the webpage I linked to) I select the points from a small (10x10 I think) window, which I suspect
necessitates a lot of exact computations/drilling down to determine which side of lines points lie on (etc) when building arrangements.
It's been a while, but that's the impression I remember having anyway.

I used Ubuntu Linux. Definitely room for improvement and extensions (e.g. the _k_-point algorithm from "Generalised k-Steiner Tree Problems in Normed Planes"
by Brazil et al. 2015)! There may be bugs. When I wrote and tested this, 
I didn't have another program to compare/test against, so as a sanity test I output all of the alternative candidates from the 
OOVD regions and was content if the shortest candidate tree wins (which it does).

I considered adding my website code to a repo as well since it wasn't easy getting it all to work together over web servers and 
using Geogebra with javascript as my display, 
(and I still might someday) but I don't know enough about web security to be able to say that I wouldn't be creating problems by exposing the code to the world.
Note that running this code is resource intensive, so it's not something you want to allow everyone in the world to run on your machine 
(which is why I've password-protected the webpage).
