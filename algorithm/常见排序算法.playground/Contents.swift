import UIKit

var str = "Hello, playground"


// Reference from https://www.cnblogs.com/onepixel/p/7674659.html

/*
 冒泡排序是一种简单的排序算法。它重复地走访过要排序的数列，一次比较两个元素，如果它们的顺序错误就把它们交换过来。走访数列的工作是重复地进行直到没有再需要交换，也就是说该数列已经排序完成。这个算法的名字由来是因为越小的元素会经由交换慢慢“浮”到数列的顶端。
 
 1.比较相邻的元素。如果第一个比第二个大，就交换它们两个；
 2.对每一对相邻元素作同样的工作，从开始第一对到结尾的最后一对，这样在最后的元素应该会是最大的数；
 3.针对所有的元素重复以上的步骤，除了最后一个；
 4.重复步骤1~3，直到排序完成。
 */
func bubbleSort<T:Comparable>(_ arr:[T]) -> [T]
{
    var r = arr
    for i in 0..<r.count - 1 { // i表示每趟 ：如果是0..<count 即需要 count次冒泡，实际上只需要count-1次冒泡即可
        for j in 0..<r.count - i - 1 {  // j，j+1表示每个相邻元素， 因为需要取到j+1故这里需要-1, 否则r[j+1]越界
            if r[j] > r[j+1] {
                let temp = r[j+1]
                r[j+1] = r[j]
                r[j] = temp
            }
        }
    }
    return r
}


var input = [5,2,1,3,7,5,6,5,4,6]
print("bubble sort input:\(input) , output:\(bubbleSort(input))")



/*
 选择排序(Selection-sort)是一种简单直观的排序算法。它的工作原理：首先在未排序序列中找到最小（大）元素，存放到排序序列的起始位置，然后，再从剩余未排序元素中继续寻找最小（大）元素，然后放到已排序序列的末尾。以此类推，直到所有元素均排序完毕。
 
 n个记录的直接选择排序可经过n-1趟直接选择排序得到有序结果。具体算法描述如下：

 1.初始状态：无序区为R[1..n]，有序区为空；
 2.第i趟排序(i=1,2,3…n-1)开始时，当前有序区和无序区分别为R[1..i-1]和R(i..n）。该趟排序从当前无序区中-选出关键字最小的记录 R[k]，将它与无序区的第1个记录R交换，使R[1..i]和R[i+1..n)分别变为记录个数增加1个的新有序区和记录个数减少1个的新无序区；
 3.n-1趟结束，数组有序化了。
 */

func selectionSort<T:Comparable>(_ arr:[T]) -> [T]
{
    var r = arr
    var minIndex:Int // 记录最小元素值的下标
    var temp:T // 原地交换元素
    
    /*每次确定第i个元素的值，即从i+1开始往后找往后找最小元素（i之前的元素已经完成排序，i是正在计算中的应该放入的元素下标，i+1开始到数组末尾都是未排序的），*/
    for i in 0..<r.count - 1 { // 注意j从i+1开始，这里需要count-1
        minIndex = i // 假设i为最小元素下标
        for j in i+1..<r.count { // 从i+1开始到末尾找最小值元素下标
            if r[j] < r[minIndex] {
                minIndex = j
            }
        }
        
        // 至此已经找到对于i下标这个元素，适合放入的元素值的下标
        // 然后交换i和minIndex下标的元素值
        temp = r[minIndex]
        r[minIndex] = r[i]
        r[i] = temp
        
        // 至此已经完成下标为i这个元素的排序
    }
    return r
}
print("selection sort input:\(input) , output:\(selectionSort(input))")



/*
 插入排序（Insertion-Sort）的算法描述是一种简单直观的排序算法。它的工作原理是通过构建有序序列，对于未排序数据，在已排序序列中从后向前扫描，找到相应位置并插入。
 
 一般来说，插入排序都采用in-place（原地排序）在数组上实现。具体算法描述如下：

 1.从第一个元素开始，该元素可以认为已经被排序；
 2.取出下一个元素，在已经排序的元素序列中从后向前扫描；
 3.如果该元素（已排序）大于新元素，将该元素移到下一位置；
 4.重复步骤3，直到找到已排序的元素小于或者等于新元素的位置；
 5.将新元素插入到该位置后；
 6.重复步骤2~5。
 
 由于是in-place排序（仅使用O(1)的空间，原地排序），因此需要反复把已排序元素逐步往后挪，为新元素插入提供空间
 */
func insertionSort<T:Comparable>(_ arr:[T]) -> [T]
{
    var r = arr
    var current:T,preIndex:Int
    for i in 1..<arr.count {
        current = r[i] // 获取当前进行排序中的元素
        preIndex = i-1 // 获取它的前一下标（前面的元素都是有序的）
        while preIndex >= 0 && r[preIndex] > current { // 从后往前寻找合适的位置（preIndex），没找到就将比较的元素往后搬移
            r[preIndex+1] = r[preIndex] // 将元素往后搬移
            preIndex -= 1 // 继续往前移动下标
        }
        // 找到合适的index位置（preIndex+1）放入r[i]的值
        r[preIndex + 1] = current
    }
    return r
}
print("insertion sort input:\(input) , output:\(insertionSort(input))")


/*
 归并排序是建立在归并操作上的一种有效的排序算法。该算法是采用分治法（Divide and Conquer）的一个非常典型的应用。将已有序的子序列合并，得到完全有序的序列；即先使每个子序列有序，再使子序列段间有序。若将两个有序表合并成一个有序表，称为2-路归并。
 
 1.把长度为n的输入序列分成两个长度为n/2的子序列；
 2.对这两个子序列分别采用归并排序；
 3.将两个排序好的子序列合并成一个最终的排序序列。
 */


func mergeSort<T:Comparable>(_ arr:[T]) ->[T]
{
    
    if arr.count < 2 { // 递归结束条件 否则下面的mergeSort 无限往下进行下去
        return arr
    }
    
    let mid = arr.count / 2          // 取一个中点
    let left = Array(arr[0..<mid])
    let right = Array(arr[mid...])
    return merge(mergeSort(left), mergeSort(right)) // 根据函数定义 此时mergeSort(left),mergeSort(right)已经是有序的数组
}

// 合并两个有序数组
func merge<T:Comparable>(_ left:[T] ,_ right:[T]) -> [T]
{
    var l = left ,r = right
    var back = [T]()

    // 不断对l,r数组取出首元素值进行比较，将最小的那个元素值拼接到返回数组尾部，条件是直到其中一个数组被取空了
    while l.count > 0 && r.count > 0 {
        if let lf = l.first,let rf = r.first {
            if lf <= rf {
                back.append(lf)
                l.removeFirst()
            }
            else{
                back.append(rf)
                r.removeFirst()
            }
        }
    }
    
    // 然后对l,r中剩下的依次拼接到返回数组尾部，因为l,r已经各自有序，所以剩下的部分肯定是有序的，直接拼接进去即可
    while l.count > 0 {
        back.append(l.removeFirst())
    }
    while r.count > 0 {
        back.append(r.removeFirst())
    }
    return back
}

print("merge sort input:\(input) , output:\(mergeSort(input))")



/*
 快速排序的基本思想：通过一趟排序将待排记录分隔成独立的两部分，其中一部分记录的关键字均比另一部分的关键字小，则可分别对这两部分记录继续进行排序，以达到整个序列有序。
 
 快速排序使用分治法来把一个串（list）分为两个子串（sub-lists）。具体算法描述如下：

 从数列中挑出一个元素，称为 “基准”（pivot）；
 重新排序数列，所有元素比基准值小的摆放在基准前面，所有元素比基准值大的摆在基准的后面（相同的数可以到任一边）。在这个分区退出之后，该基准就处于数列的中间位置。这个称为分区（partition）操作；
 递归地（recursive）把小于基准值元素的子数列和大于基准值元素的子数列排序。
 */
func quickSort<T:Comparable>(_ arr:inout [T],_ left:Int,_ right:Int)
{
    if left < right {
        let p = partition(&arr, left, right)
        quickSort(&arr, left, p-1)
        quickSort(&arr, p+1, right)
    }
}

// 寻找arr数组left，right的基点p
func partition<T:Comparable>(_ arr:inout [T], _ left:Int , _ right:Int) -> Int
{
    /*
     步骤：
     1.寻找arr 在left和right下标范围内的主元:【主元的选择需要有技巧，是为了方便进行数组中元素比较及位置交换】
     2.然后对arr中的元素进行交换【大于主元的元素放到主元右边，小于主元的元素放到主元左边,等于主元的元素可放到任意边】，注意这里需要原地交换元素，是in-place操作
     */
    
    // 【关键：寻找主元的方式】取right下标元素为主元
    // 基于基点p的数组分隔 partition算法有点双指针的味道，i维护小于p的部分，j维护了大于p的部分，过程是i不断往后递增的过程【发现arr[j]小于主元，需要放入i维护的数组中】中可能会侵占j维护的的首部，而这部分恰好可通过交换当前j所指元素和i即将侵占【下标i+1】的元素达到要求
    // 最后只剩主元p时，只需交换j维护的部分内【i+1可满足】和主元即可
    var i = left - 1
    for j in left..<right {
        if arr[j] <= arr[right] {
            i += 1
            arr.swapAt(j, i)
        }
    }
    i += 1
    arr.swapAt(i, right)
    return i
}

print("quick sort input:\(input)")
quickSort(&input, 0, input.count-1)
print("quick sort output:\(input)")



/*
 堆排序
 堆排序（Heapsort）是指利用堆这种数据结构所设计的一种排序算法。堆积是一个近似完全二叉树的结构，并同时满足堆积的性质：即子结点的键值或索引总是小于（或者大于）它的父节点。
 1.将初始待排序关键字序列(R1,R2….Rn)构建成大顶堆，此堆为初始的无序区；
 2.将堆顶元素R[1]与最后一个元素R[n]交换，此时得到新的无序区(R1,R2,……Rn-1)和新的有序区(Rn),且满足R[1,2…n-1]<=R[n]；
 3.由于交换后新的堆顶R[1]可能违反堆的性质，因此需要对当前无序区(R1,R2,……Rn-1)调整为新堆，然后再次将R[1]与无序区最后一个元素交换，得到新的无序区(R1,R2….Rn-2)和新的有序区(Rn-1,Rn)。不断重复此过程直到有序区的元素个数为n-1，则整个排序过程完成。
 
 关键点
 1.如何构造最大堆
 2.如何维持最大堆性质
 3.堆中数组是从下标1开始【为了每个节点方便计算其父，左，右孩子下标序号】
 */


// 表示堆的数组a包括两个属性，a.count【通常】给出数组元素的个数，a.heapSize表示了有多少个堆元素存储在该数组中，也就是说虽然[1...a.count]可能都有存数据，但只有[1...a.heapSize]存放的是堆的有效元素
// 这里 0<= a.heapSize <= a.count

var heapSize:Int = 0 // 堆的在数组arr中的有效存储大小

func heapSort<T:Comparable>(_ arr:inout [T])
{
    // 1.构建最大堆
    buildMaxHeap(&arr)
    
    // 2.循环从最大堆中取出根节点与尾部的元素交换【非最后一个元素】 ，并继续维持最大堆性质
    for i in (1..<arr.count).reversed() {
        arr.swapAt(0, i)
        heapSize -= 1
        maxHeapify(&arr, 0)
    }
}

// 构建最大堆
func buildMaxHeap<T:Comparable>(_ arr:inout [T])
{
    heapSize = arr.count // 最开始构建最大堆时，使用数组的大小，随着不断交换堆顶元素出堆外，heapSize在不断减小
    
    for i in (0...arr.count/2).reversed() { // 二叉堆最后一层全部为叶子结点，可看作已经是满足最大堆性质，另外最大堆性质需自底向上递归维护进行
        maxHeapify(&arr, i)
    }
    print("max heap :\(arr)")
}

// inline macro define needed
func left(_ root:Int)->Int
{
    root * 2 + 1
}
func right(_ root:Int)->Int
{
    root * 2 + 2
}

// 维持以root为根结点的当前堆【指以root为根的当前最小堆结构】的最大堆性质
func maxHeapify<T:Comparable>(_ arr:inout [T],_ root:Int)
{
    
    var largest = root
    
    if left(root) < heapSize && arr[largest] < arr[left(root)] { // 注意这里不是left(root) < arr.count,而是使用堆在数组中的有效存储大小heapSize
        largest = left(root)
    }
    
    if right(root) < heapSize && arr[right(root)] > arr[largest] { // 同上
        largest = right(root)
    }
    
    // 当前节点非最大堆性质，除了交换当前节点和子树的位置，还需要对其子树递归维护其最大堆性质
    if largest != root {
        arr.swapAt(largest, root)
        maxHeapify(&arr, largest) // 递归进行子树的最大堆性质维护
    }
}

var input2 = [5,2,1,3,7,5,6,5,4,6]
print("Heap sort input:\(input2)")
heapSort(&input2)
print("Heap sort output:\(input2)")
