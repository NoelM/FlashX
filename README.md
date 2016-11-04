FlashX is a collection of big data analytics tools that perform data analytics
in the form of graphs and matrices. They utilize solid-state drives (SSDs) to
scale to large datasets in a single machine. It has five main components:
SAFS, FlashGraph, FlashMatrix, FlashEigen and FlashR.

SAFS
========

SAFS a user-space filesystem designed for large SSD arrays. It is capable of
achieving the maximal I/O throughput from SSD arrays (millions of I/O per
second for random I/O and tens of gigabytes per second for sequential I/O).
The I/O performance is further magnified by the light-weight page cache if
the workload can generate cache hits.

FlashGraph
===========

FlashGraph is a general-purpose graph analysis framework that exposes
vertex-centric programming interface for users to express varieties of
graph algorithms. FlashGraph scales graph computation to large graphs by
keeping the edges of a graph on SSDs and computation state in memory.
With smart I/O scheduling, FlashGraph is able to achieve performance
comparable to state-of-art in-memory graph analysis frameworks and
significantly outperforms state-of-art distributed graph analysis frameworks
while being able to scale to graphs with billions of vertices and hundreds
of billions of edges. Please see
[the performance result](https://github.com/zheng-da/FlashX/wiki/FlashX-performance-and-scalability).

FlashMatrix
===========

FlashMatrix is a matrix computation engine that provides a small set of
generalized matrix operations on sparse matrices and dense matrices to express
varieties of data mining and machine learning algorithms. For certain graph
algorithms such as PageRank, which can be formulated as sparse matrix
multiplication, FlashMatrix is able to significantly outperform FlashGraph.

FlashEigen
==========
FlashEigen is an eigensolver that extends the
[Anasazi](https://trilinos.org/packages/anasazi/) eigensolvers with FlashMatrix.
It computes eigenvalues of billion-node graphs efficiently and pricisely
in a single machine. As such, FlashEigen enables users to perform spectral
analysis on very large graphs in a single machine.

FlashR
===========

FlashR extends the existing R programming framework to process datasets at
a scale of terabytes. FlashR integrates the matrices and generalized operators
of FlashMatrix with R so that R users can implement many data mining and
machine learning algorithms completely in R with performance comparable
to optimized C implementations. FlashR reimplements many existing R functions
for matrices to provide users a familiar R programming environment. FlashR
is implemented as a regular R package.
The [programming tutorial](https://github.com/icoming/FlashX/wiki/FlashR-programming-tutorial)
shows all of the features in FlashR.

Documentation
========

[FlashX Quick start guide](https://github.com/icoming/FlashX/wiki/FlashX-Quick-Start-Guide)

[FlashGraph programming tutorial](https://github.com/icoming/FlashX/wiki/FlashGraph-programming-tutorial).

[FlashR programming tutorial](https://github.com/icoming/FlashX/wiki/FlashR-programming-tutorial)

[FlashX performance and scalability](https://github.com/zheng-da/FlashX/wiki/FlashX-performance-and-scalability)

[SAFS user manual](https://github.com/icoming/FlashGraph/wiki/SAFS-user-manual).

Publications
========
Da Zheng, Disa Mhembere, Joshua T. Vogelstein, Carey E. Priebe, and Randal Burns, “FlashMatrix: Parallel, scalable data analysis with generalized matrix operations using commodity ssds,” arXiv preprint arXiv:1604.06414, 2016 [[pdf](http://arxiv.org/pdf/1604.06414v3)]

Da Zheng, Disa Mhembere, Vince Lyzinski, Joshua Vogelstein, Carey E. Priebe, and Randal Burns, “Semi-external memory sparse matrix multiplication on billion-node graphs”, Transactions on Parallel and Distributed Systems, 2016. [[pdf](https://arxiv.org/pdf/1602.02864v3.pdf)]

Heng Wang, Da Zheng, Randal Burns, Carey Priebe, Active Community Detection in Massive Graphs, SDM-Networks 2015 [[pdf](http://arxiv.org/pdf/1412.8576v3.pdf)]

Da Zheng, Disa Mhembere, Randal Burns, Joshua Vogelstein, Carey E. Priebe, Alexander S. Szalay, FlashGraph: Processing Billion-Node Graphs on an Array of Commodity SSDs, FAST'15, [[pdf](https://www.usenix.org/system/files/conference/fast15/fast15-paper-zheng.pdf)][[bib](https://www.usenix.org/biblio/export/bibtex/188418)]

Da Zheng, Randal Burns, Alexander S. Szalay, Toward Millions of File System IOPS on Low-Cost, Commodity Hardware, in Proceeding SC '13 Proceedings of the International Conference on High Performance Computing, Networking, Storage and Analysis, [[pdf](http://www.cs.jhu.edu/~zhengda/sc13.pdf)][[bib](http://dl.acm.org/downformats.cfm?id=2503225&parent_id=2503210&expformat=bibtex&CFID=445591569&CFTOKEN=95321450)]

Contact
========

Mailing list: flashgraph-user@googlegroups.com

[![Join the chat at https://gitter.im/icoming/FlashGraph](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/icoming/FlashGraph?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
