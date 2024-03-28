import pyNUISANCE as pn
import pyProSelecta as pps
import matplotlib.pyplot as plt


class comparison_guru:
    def __init__(self):
        self.rfact = pn.RecordFactory()
        self.input_source_mapping = {} # avoid dup inputs
        self.input_table_mapping = {} # map each input to table list
        self.table_comparisons = {} 

        self.labels = []
        self.tables = {}
        self.comparisons = {}
        self.entries = {}

        self.source_objects = {}
        self.source_mapping = {}
        self.source_weights = {}
        
        self.wfact = pn.WeightCalcFactory()
        self.dial_values = {}
        
    def add(self, yaml_obj, label=None):
        
        table = self.rfact.make_table( yaml_obj )
        if not label: label = table.metadata()["id"]
        
        self.labels.append(label)
        self.entries[label] = yaml_obj
        self.tables[label] = table
        self.comparisons[label] = table.comparison()

        filepath = yaml_obj["filepath"]
        if filepath not in self.source_objects:
            self.source_objects[filepath] = pn.EventSource(filepath)
            self.source_weights[filepath] = self.wfact.make(self.source_objects[filepath], {"neut_cardname": "neut.card"})
            self.source_mapping[filepath] = []

        self.source_mapping[filepath].append( [self.tables[label],self.comparisons[label]] )

    def set_parameter(self, dial, value):
        self.dial_values[dial] = value
        
    def add_hepdata(self, release, table, input_file):
        yaml_obj = {}
        yaml_obj["type"] = "hepdata"
        yaml_obj["filepath"] = input_file
        yaml_obj["release"] = release
        yaml_obj["table"] = table
        self.add(yaml_obj)
        
    def eval(self):

        for weighter in self.source_weights.values():
            weighter.set_parameters(self.dial_values)
            
        for tab, comp in zip(self.tables.values(), 
                             self.comparisons.values()):
            tab.clear(comp)

        for source, associations, weighter in zip(
            self.source_objects.values(),
            self.source_mapping.values(),
            self.source_weights.values()):

            count = 0
            for ev, cvw in source:  
                
                engine_weight = 1.0
                engine_weight *= weighter.calc_weight(ev)
                    
                for tab, comp in associations:
                    comp.mc.fill_if(tab.select(ev),
                                    tab.project(ev),
                                    cvw * engine_weight * tab.weight(ev))

                count += 1
                if count % 1000 == 0: print(count)
                if count > 10000: break
                    
            for tab, comp in associations:
                tab.finalize(comp, source.fatx()/source.sumw())
            
        chi2 = 0
        for tab, comp in zip(self.tables.values(), 
                             self.comparisons.values()):
            chi2 += tab.likelihood(comp)

        return chi2

    def plot(self):
        for comp in self.comparisons.values():
            plt.title(comp.metadata["id"])
            print(comp.data)
            comp.data.mpl().errorbar(label="Data", color='black', ls='none')
            comp.estimate.mpl().hist(color='green', alpha=0.5, label="MC")  
            plt.legend()
            plt.show()
            

cg = comparison_guru()
cg.add_hepdata("ANL/CCQE/182176/", "EventCounts-Q2", "neut.root")
cg.set_parameter("AxlFFCCQE", 2)
cg.set_parameter("MaCCQE", 0.0)
cg.eval()
cg.plot()
