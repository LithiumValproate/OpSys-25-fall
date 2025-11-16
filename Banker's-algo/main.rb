def vec_leq?(a, b)
  a.zip(b).all? { |x, y| x <= y }
end

def vec_add(a, b)
  a.zip(b).map { |x, y| x + y }
end

def vec_sub(a, b)
  a.zip(b).map { |x, y| x - y }
end

SystemState = Struct.new(:total, :available, :max, :allocation, :need) do
  def n
    max.length;
  end
  
  def m
    total.length;
  end
end

class Banker
  attr_reader :state
  
  def initialize(state)
    @state = state
  end
  
  def safe?
    n        = state.n
    work     = state.available.dup
    finish   = Array.new(n, false)
    sequence = []
    logs     = ["初始 Work = #{work.inspect}"]
    
    loop do
      progress = false
      n.times do |i| next if finish[i]
      
      next unless vec_leq?(state.need[i], work)
      
      logs << "P#{i} 可满足 Need=#{state.need[i].inspect} ≤ Work=#{work.inspect}，执行并释放 Allocation=#{state.allocation[i].inspect}"
      work      = vec_add(work, state.allocation[i])
      finish[i] = true
      sequence << i
      logs << "执行后 Work=#{work.inspect}"
      progress = true
      end
      
      break unless progress
    end
    
    safe = finish.all?
    if safe
      logs << "系统处于安全状态。安全序列：" + sequence.map { |i| "P#{i}" }.join(" → ")
    else remain = finish.each_with_index.map { |f, i| f ? nil : "P#{i}" }.compact.join(', ')
    logs << "系统不安全，未完成进程：#{remain}"
    end
    
    [safe, sequence, logs]
  end
  
  def request(pid, req)
    return [false, ["非法进程号 P#{pid}"]] unless (0...state.n).cover?(pid)
    return [false, ["非法请求：维度错误"]] unless req.length == state.m
    return [false, ["非法请求：含负数"]] if req.any? { |x| x.negative? }
    
    logs = ["收到请求：P#{pid} 请求 #{req.inspect}"]
    
    unless vec_leq?(req, state.need[pid])
      logs << "拒绝：请求超过 Need。Need=#{state.need[pid].inspect}, Req=#{req.inspect}"
      return [false, logs]
    end
    
    unless vec_leq?(req, state.available)
      logs << "拒绝：请求超过 Available。Available=#{state.available.inspect}, Req=#{req.inspect}"
      return [false, logs]
    end
    
    logs << "通过合法性检查，预分配并进行安全性检查…"
    old_availability = state.available.dup
    old_allocation   = state.allocation[pid].dup
    old_need         = state.need[pid].dup
    
    state.available       = vec_sub(state.available, req)
    state.allocation[pid] = vec_add(state.allocation[pid], req)
    state.need[pid]       = vec_sub(state.need[pid], req)
    
    safe, _seq, safety_logs = safe?
    logs << "——[安全性检查开始]——"
    logs.concat(safety_logs)
    logs << "——[安全性检查结束]——"
    
    if safe
      logs << "批准：分配后系统安全"
      [true, logs]
    else state.available  = old_availability
    state.allocation[pid] = old_allocation
    state.need[pid]       = old_need
    logs << "拒绝并回滚：分配后不安全"
    [false, logs]
    end
  end
end

def pretty_state(s)
  out = []
  out << "\n===== 当前系统状态 ====="
  out << "资源种类 m=#{s.m}，进程数 n=#{s.n}"
  out << "Total     = #{s.total.inspect}"
  out << "Available = #{s.available.inspect}"
  [["Max", s.max], ["Allocation", s.allocation], ["Need", s.need]].each do |title, mat| out << "\n[#{title}]"; out << ("P\\R\t" + (0...s.m).map { |j| "R#{j}\t" }.join)
  s.n.times { |i| out << ("P#{i}\t" + mat[i].map { |x| x.to_s + "\t" }.join) }
  end
  out.join("\n")
end

def sample_state
  total = [10, 5, 7]
  max   = [[7, 5, 3], [3, 2, 2], [9, 0, 2], [2, 2, 2], [4, 3, 3]]
  alloc = [[0, 1, 0], [2, 0, 0], [3, 0, 2], [2, 1, 1], [0, 0, 2]]
  n     = max.length; m = total.length
  need  = Array.new(n) { Array.new(m, 0) }
  sum   = Array.new(m, 0)
  n.times { |i| m.times { |j| need[i][j] = max[i][j] - alloc[i][j]; sum[j] += alloc[i][j] } }
  avail = (0...m).map { |j| total[j] - sum[j] }
  SystemState.new(total, avail, max, alloc, need)
end

def build_from_input
  puts "\n=== 初始化系统 ==="
  n = m = 0
  loop do
    print "请输入进程数量 n (>=5)："
    n = STDIN.gets.to_i
    print "请输入资源种类 m (>=3)："
    m = STDIN.gets.to_i
    break if n >= 5 && m >= 3
    puts "要求：n>=5 且 m>=3。请重新输入。\n"
  end
  
  total = nil
  loop do
    print "Total (m=#{m} 项)："
    parts = STDIN.gets.to_s.split.map(&:to_i)
    if parts.length == m && parts.all? { |x| x >= 0 }
      total = parts
      break
    end
    puts "格式错误，需输入 m 个非负整数。"
  end
  
  max   = Array.new(n) { Array.new(m, 0) }
  alloc = Array.new(n) { Array.new(m, 0) }
  
  n.times do |i| loop do
    print "P#{i} Max (m=#{m})："
    v = STDIN.gets.to_s.split.map(&:to_i)
    if v.length == m && v.all? { |x| x >= 0 }
      max[i] = v
      break
    end
    puts "格式错误，需输入 m 个非负整数。"
  end
  
  loop do
    print "P#{i} Allocation (m=#{m})："
    v = STDIN.gets.to_s.split.map(&:to_i)
    if v.length == m && v.all? { |x| x >= 0 } && vec_leq?(v, max[i])
      alloc[i] = v
      break
    end
    puts "格式错误或超过 Max，请重输。"
  end
  end
  
  need = max.each_with_index.map { |mx, i| vec_sub(mx, alloc[i]) }
  mcol = Array.new(m, 0)
  alloc.each { |row| row.each_with_index { |x, j| mcol[j] += x } }
  avail = total.each_with_index.map { |t, j| t - mcol[j] }
  raise "初始化非法：分配超过总量" if avail.any? { |x| x < 0 }
  SystemState.new(total, avail, max, alloc, need)
end

puts "银行家算法模拟器 (Ruby)"
print "1) 手动输入  2) 示例数据  选择："
choice = STDIN.gets.to_s.strip
s      = (choice == '2') ? sample_state : build_from_input
banker = Banker.new(s)

loop do
  puts "\n=============== 菜单 ==============="
  puts "1. 显示系统状态\n2. 安全性检查\n3. 处理资源请求\n4. 退出"
  print "请选择操作："
  op = STDIN.gets.to_s.strip
  case op
    when '1'
      puts pretty_state(banker.state)
    when '2'
      _safe, _seq, logs = banker.safe?
      puts "\n" + logs.join("\n")
    when '3'
      print "输入 pid："
      pid = STDIN.gets.to_i
      m   = banker.state.m
      print "输入请求向量 (m=#{m})："
      req       = STDIN.gets.to_s.split.map(&:to_i)
      _ok, logs = banker.request(pid, req)
      puts "\n" + logs.join("\n")
    when '4'
      puts "已退出。"
      break
    else puts "无效选择。"
  end
end
