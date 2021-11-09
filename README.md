Disk-based-BACF-Tree
====================

This codes consist of 4 folder. 
-------------------------------

### 1. BACF Tree 
##### This Folder consists of codes that build Buffer Augmented Clustering Features Tree for Data. 
##### After building BACF Tree, LeafNodeInfo.txt is result file.
##### The result file is composed in the form below. 
##### Paper : Under submission to the Korean Software Congress 2021. [update when the results come out]

```
Data Index in Each Index | MC(Micro Cluster)'s centroid coordinates 
Ex) 990317/990421/993267/994960/995893/996624/999388|7788.288679 7920.202520 
```

### 2. BIRCH CF Tree
##### This Folder consists of codes that build Birch's Clustering Features Tree for Data. 
##### Birch's CF Tree is based on [BIRCH: An Efficient Data Clustering Method for Very Large Databases Paper.](https://www2.cs.sfu.ca/CourseCentral/459/han/papers/zhang96.pdf)
##### I used the Birch's CF Tree building code from [here](https://github.com/sehee-lee/JBIRCH)
##### I changed some of calculating memory size function in that code. 
##### After building CF Tree, InputFileName_leaf_.txt is result file.
##### The result file is composed in the form below. 
```
MC(Micro Cluster)'s centroid coordinates|[Data Index in Each Index] 
Ex) 5896.839721 3043.506218384615|[2776, 3178, 3409, 3258, 5229, 2998]
```

### 3. DATA GENERATOR 
##### This Folder consists of code that generates data with random pattern cluster

### 4. K-MEANS CLUSTERING 
##### This Folder consists of codes that proceeds K-MEANS clustering with result file made in BACF Tree and BIRCH's CF Tree. 
