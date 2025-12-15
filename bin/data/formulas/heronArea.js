// 海伦公式计算三角形面积 (a, b, c: 三边长)
function(a, b, c) { return 0.25 * Math.sqrt((a+b+c)*(a+b-c)*(a+c-b)*(b+c-a)); }