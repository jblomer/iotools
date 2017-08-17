import java.io.File;
import java.io.IOException;
import lhcb.cern.ch.Event;
import org.apache.avro.io.DatumReader;
import org.apache.avro.specific.SpecificDatumReader;
import org.apache.avro.file.DataFileReader;

class lhcbOpenData
{
  public static void main(String args[]) throws IOException {
    DatumReader<Event> datumReader =
      new SpecificDatumReader<Event>(Event.class);
    DataFileReader<Event> fileReader =
      new DataFileReader<Event>(
        new File(args[0]),
        datumReader);
    Event event = null;
    int i = 0;
    int skipped = 0;
    double sum = 0.0;
    while (fileReader.hasNext()) {
      event = fileReader.next(event);
      ++i;
      if ((i % 100000) == 0) {
        System.out.printf("Processed %d k events\n", i / 1000);
      }

      if (event.getH1IsMuon() == 1 || event.getH2IsMuon() == 1 ||
          event.getH3IsMuon() == 1)
      {
        skipped++;
        continue;
      }

      sum += event.getH1Px() +
             event.getH1Py() +
             event.getH1Pz() +
             event.getH1ProbK() +
             event.getH1ProbPi() +
             (double)(event.getH1Charge()) +
             event.getH2Px() +
             event.getH2Py() +
             event.getH2Pz() +
             event.getH2ProbK() +
             event.getH2ProbPi() +
             (double)(event.getH2Charge()) +
             event.getH3Px() +
             event.getH3Py() +
             event.getH3Pz() +
             event.getH3ProbK() +
             event.getH3ProbPi() +
             (double)(event.getH3Charge());
    }

    System.out.printf("finished (%d events), result: %f, skipped: %d\n",
                      i, sum, skipped);
  }
}
