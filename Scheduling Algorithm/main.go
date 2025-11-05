package main

import (
	"fmt"
	"sort"
	"strconv"
	"strings"
)

// --- Gantt chart printing ---

type Segment struct {
	Start int
	End   int
	Name  string
}

func PrintGantt(segments []Segment) {
	if len(segments) == 0 {
		fmt.Println("(empty gantt)")
		return
	}
	var scale strings.Builder
	var bars strings.Builder
	last := segments[0].Start
	for _, s := range segments {
		if s.Start > last {
			gap := s.Start - last
			scale.WriteString(strings.Repeat("    ", gap))
			bars.WriteString(strings.Repeat("    ", gap))
		}
		width := s.End - s.Start
		cell := fmt.Sprintf("[%s]", centerString(s.Name, max(1, width*2)))
		bars.WriteString(cell)
		scale.WriteString(fmt.Sprintf("%-4d", s.Start))
		last = s.End
	}
	scale.WriteString(fmt.Sprintf("%-4d", segments[len(segments)-1].End))
	fmt.Println(scale.String())
	fmt.Println(bars.String())
}

func centerString(s string, width int) string {
	if len(s) >= width {
		return s[:width]
	}
	left := (width - len(s)) / 2
	right := width - len(s) - left
	return strings.Repeat(" ", left) + s + strings.Repeat(" ", right)
}

// --- Schedulable object ---

type Schedulable interface {
	Name() string
	Arrival() int
	Burst() int
	Start() int
	Finish() int
	SetStart(int)
	SetFinish(int)
	ComputeStats()
	Wait() int
	Tat() int
}

type InputInfo struct {
	id      int
	name    string
	Arrival int
	Burst   int
}

func (i InputInfo) Name() string {
	return strconv.Itoa(i.id) + "-" + i.name
}

type BaseInfo struct {
	InputInfo
	start  int
	finish int
	wait   int
	tat    int
}

func NewBaseInfo(id int, name string, arrival int, burst int) BaseInfo {
	return BaseInfo{
		InputInfo: InputInfo{
			id:      id,
			name:    name,
			Arrival: arrival,
			Burst:   burst,
		},
		start:  -1,
		finish: -1,
		wait:   -1,
		tat:    -1,
	}
}

func (b BaseInfo) Arrival() int { return b.InputInfo.Arrival }
func (b BaseInfo) Burst() int   { return b.InputInfo.Burst }
func (b BaseInfo) Start() int   { return b.start }
func (b BaseInfo) Finish() int  { return b.finish }
func (b BaseInfo) Wait() int    { return b.wait }
func (b BaseInfo) Tat() int     { return b.tat }

type Job struct {
	BaseInfo
	Priority int
}

func NewJob(id int, name string, arrival int, burst int) *Job {
	return &Job{
		BaseInfo: NewBaseInfo(id, name, arrival, burst),
		Priority: 0,
	}
}

func (j *Job) SetStart(s int) { j.start = s }
func (j *Job) SetFinish(f int) {
	j.finish = f
	j.ComputeStats()
}
func (j *Job) ComputeStats() {
	if j.finish >= 0 {
		j.tat = j.finish - j.Arrival()
		j.wait = j.tat - j.Burst()
		if j.wait < 0 {
			j.wait = 0
		}
	}
}

func (j *Job) SetPriority(p int) { j.Priority = p }

type Pcb struct {
	BaseInfo
	Remain   int
	restarts []int
}

func NewPcb(id int, name string, arrival int, burst int) *Pcb {
	return &Pcb{
		BaseInfo: NewBaseInfo(id, name, arrival, burst),
		Remain:   burst,
		restarts: nil,
	}
}

func (p *Pcb) SetStart(s int) {
	if p.start == -1 {
		p.start = s
	}
	p.restarts = append(p.restarts, s)
}
func (p *Pcb) SetFinish(f int) {
	p.finish = f
	p.Remain = 0
	p.ComputeStats()
}
func (p *Pcb) ComputeStats() {
	if p.finish >= 0 {
		p.tat = p.finish - p.Arrival()
		p.wait = p.tat - p.Burst()
		if p.wait < 0 {
			p.wait = 0
		}
	}
}

// Schedulable interface compliance checks
var _ Schedulable = (*Job)(nil)
var _ Schedulable = (*Pcb)(nil)

// --- Scheduling algorithms ---

func FCFS(items []Schedulable) ([]Segment, []string) {
	if len(items) == 0 {
		return nil, []string{"No items to schedule"}
	}
	copied := make([]Schedulable, len(items))
	copy(copied, items)
	sort.Slice(copied, func(i, j int) bool {
		if copied[i].Arrival() != copied[j].Arrival() {
			return copied[i].Arrival() < copied[j].Arrival()
		}
		return copied[i].Name() < copied[j].Name()
	})
	var (
		gantt []Segment
		logs  []string
	)
	t := copied[0].Arrival()
	for _, it := range copied {
		startT := max(t, it.Arrival())
		it.SetStart(startT)
		finishT := startT + it.Burst()
		it.SetFinish(finishT)
		gantt, logs = resOut(it, gantt, logs, nil)
		t = it.Finish()
	}
	return gantt, logs
}

func SJF(items []Schedulable) ([]Segment, []string) {
	if len(items) == 0 {
		return nil, []string{"No items to schedule"}
	}
	copied := make([]Schedulable, len(items))
	copy(copied, items)
	sort.Slice(copied, func(i, j int) bool {
		if copied[i].Arrival() != copied[j].Arrival() {
			return copied[i].Arrival() < copied[j].Arrival()
		}
		return copied[i].Name() < copied[j].Name()
	})
	var (
		gantt      []Segment
		logs       []string
		remaining  = len(copied)
		nextIdx    = 0
		readyQueue []Schedulable
	)
	t := copied[0].Arrival()
	for remaining > 0 {
		for nextIdx < len(copied) && copied[nextIdx].Arrival() <= t {
			readyQueue = append(readyQueue, copied[nextIdx])
			nextIdx++
		}
		if len(readyQueue) == 0 {
			if nextIdx < len(copied) {
				t = copied[nextIdx].Arrival()
				continue
			}
			break
		}
		sort.SliceStable(readyQueue, func(i, j int) bool { return readyQueue[i].Burst() < readyQueue[j].Burst() })
		currIt := readyQueue[0]
		readyQueue = readyQueue[1:]
		startT := t
		currIt.SetStart(startT)
		finishT := startT + currIt.Burst()
		currIt.SetFinish(finishT)
		gantt, logs = resOut(currIt, gantt, logs, nil)
		t = currIt.Finish()
		remaining--
	}
	return gantt, logs
}

func SRTF(items []Schedulable) ([]Segment, []string) {
	if len(items) == 0 {
		return nil, []string{"No items to schedule"}
	}
	pcbs := make([]*Pcb, len(items))
	for i := range items {
		p, ok := items[i].(*Pcb)
		if !ok || p == nil {
			return nil, []string{fmt.Sprintf("SRTF requires *Pcb at index %d", i)}
		}
		pcbs[i] = p
	}
	sort.Slice(pcbs, func(i, j int) bool {
		if pcbs[i].Arrival() != pcbs[j].Arrival() {
			return pcbs[i].Arrival() < pcbs[j].Arrival()
		}
		return pcbs[i].Name() < pcbs[j].Name()
	})
	var (
		gantt      []Segment
		logs       []string
		readyQueue []*Pcb
		curr       *Pcb
		nextIdx    = 0
		completed  = 0
	)
	t := pcbs[0].Arrival()
	for completed < len(pcbs) {
		for nextIdx < len(pcbs) && pcbs[nextIdx].Arrival() <= t {
			readyQueue = append(readyQueue, pcbs[nextIdx])
			nextIdx++
		}
		if curr != nil {
			readyQueue = append(readyQueue, curr)
			curr = nil
		}
		if len(readyQueue) == 0 {
			if nextIdx < len(pcbs) {
				t = pcbs[nextIdx].Arrival()
				continue
			}
			break
		}
		sort.SliceStable(readyQueue, func(i, j int) bool {
			pi, pj := readyQueue[i], readyQueue[j]
			if pi.Remain != pj.Remain {
				return pi.Remain < pj.Remain
			}
			return pi.Arrival() < pj.Arrival()
		})
		curr = readyQueue[0]
		readyQueue = readyQueue[1:]
		if curr.Start() == -1 {
			curr.SetStart(t)
		}
		duration := curr.Remain
		if nextIdx < len(pcbs) {
			toNext := pcbs[nextIdx].Arrival() - t
			duration = min(toNext, curr.Remain)
		}
		param := &PreemptArgs{t, t + duration, duration}
		gantt, logs = resOut(curr, gantt, logs, param)
		t += duration
		curr.Remain -= duration
		if curr.Remain == 0 {
			curr.SetFinish(t)
			gantt, logs = resOut(curr, gantt, logs, param)
			curr = nil
			completed++
		}
	}
	return gantt, logs
}

// --- Helper functions ---

type PreemptArgs struct {
	Run, Pause, Duration int
}

func resOut(s Schedulable, g []Segment, l []string, param *PreemptArgs) ([]Segment, []string) {
	if param == nil {
		g = append(g, Segment{Start: s.Start(), End: s.Finish(), Name: s.Name()})
		l = append(l, fmt.Sprintf("t=%d: Run %s (burst=%d) -> Finish=%d", s.Start(), s.Name(), s.Burst(), s.Finish()))
	} else if p, ok := s.(*Pcb); ok {
		if p.Finish() == -1 {
			g = append(g, Segment{Start: param.Run, End: param.Pause, Name: p.Name()})
			l = append(l, fmt.Sprintf(
				"t=%d: %s runs %d unit(s). Remain %d -> %d",
				param.Run, p.Name(), param.Duration, p.Remain, p.Remain-param.Duration,
			))
		} else {
			l = append(l, fmt.Sprintf("t=%d: Process %s finished.", p.Finish(), p.Name()))
			return g, l
		}
	}
	return g, l
}

func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}

func main() {
}
